#include <mc/data/mstream.h>

#include <mc/data/list.h>
#include <mc/util/error.h>
#include <mc/util/minmax.h>

#include <memory.h>

#define BLOCK_SIZE 0x7F
#define BLOCK(CURSOR) ((CURSOR) >> 7)
#define SIZE_IN_BLOCKS(SIZE_IN_BYTES) (BLOCK((SIZE_IN_BYTES) + BLOCK_SIZE - 1))
#define OFFSET_IN_BLOCK(CURSOR) ((CURSOR) & BLOCK_SIZE)
#define SIZE_TO_BLOCK_END(CURSOR) (BLOCK_SIZE - OFFSET_IN_BLOCK(CURSOR))

typedef struct Ctx Ctx;
struct Ctx{
    MC_Alloc *alloc;

    size_t size;
    int64_t cursor;

    size_t blocks_cnt;
    uint8_t **blocks;

    struct Buffer{
        struct Buffer *next;
        uint8_t data[];
    } *g;
};

static MC_Error read(void *ctx, size_t size, void *data, size_t *read);
static MC_Error write(void *ctx, size_t size, const void *data, size_t *written);
static MC_Error get_cursor(void *ctx, size_t *cursor);
static MC_Error set_cursor(void *ctx, int64_t cursor, MC_CursorFrom from);
static void close(void *ctx);

static MC_Error alloc_more_blocks(Ctx *ctx, size_t min);
static void adjust_size(Ctx *ctx);

static MC_StreamVtab vtab = {
    .read = read,
    .write = write,
    .get_cursor = get_cursor,
    .set_cursor = set_cursor,
    .close = close
};

MC_Error mc_mstream(MC_Alloc *alloc, MC_Stream **mstream){
    Ctx ctx = {
        .alloc = alloc,
    };

    return mc_open(alloc, mstream, &vtab, sizeof(Ctx), &ctx);
}

size_t mc_mstream_size(MC_Stream *mstream){
    if(mstream == NULL){
        return 0;
    }

    Ctx *ctx = mc_ctx(mstream);
    return ctx->size;
}

static MC_Error read(void *_ctx, size_t size, void *data, size_t *ret_read){
    Ctx *ctx = _ctx;

    uint8_t *remaining_data = data;

    const size_t avail = ctx->size - ctx->cursor;
    const size_t to_read = MIN(avail, size);
    *ret_read = to_read;

    size_t off = OFFSET_IN_BLOCK(ctx->cursor);
    size_t block = BLOCK(ctx->cursor);
    if(off){
        size_t to_block_end = SIZE_TO_BLOCK_END(ctx->cursor);
        if(to_block_end >= to_read){
            memcpy(remaining_data, ctx->blocks[block] + off, to_read);
            ctx->cursor += to_read;
            return MCE_OK;
        }

        memcpy(remaining_data, ctx->blocks[block] + off, to_block_end);
        ctx->cursor += to_block_end;
        remaining_data += to_block_end;
        block++;
    }

    size_t remaining_size = to_read - (remaining_data - (uint8_t*)data);
    for(size_t last_block = BLOCK(ctx->cursor + remaining_size); block != last_block; block++){
        memcpy(remaining_data, ctx->blocks[block], BLOCK_SIZE);
        ctx->cursor += BLOCK_SIZE;
        remaining_data += BLOCK_SIZE;
    }

    remaining_size = to_read - (remaining_data - (uint8_t*)data);
    if(remaining_size){
        memcpy(remaining_data, ctx->blocks[block], remaining_size);
        ctx->cursor += remaining_size;
        remaining_data += BLOCK_SIZE;
    }

    return MCE_OK;
}

static MC_Error write(void *_ctx, size_t size, const void *data, size_t *written){
    Ctx *ctx = _ctx;

    const uint8_t *remaining_data = data;
    const size_t avail = ctx->blocks_cnt * BLOCK_SIZE - ctx->cursor;

    if(avail < size){
        size_t remaining = size - avail;
        MC_RETURN_ERROR(alloc_more_blocks(ctx, SIZE_IN_BLOCKS(remaining)));
    }

    size_t off = OFFSET_IN_BLOCK(ctx->cursor);
    size_t block = BLOCK(ctx->cursor);
    if(off){
        size_t to_block_end = SIZE_TO_BLOCK_END(ctx->cursor);
        if(to_block_end >= size){
            memcpy(ctx->blocks[block] + off, remaining_data, size);
            ctx->cursor += size;
            *written = size;
            adjust_size(ctx);
            return MCE_OK;
        }

        memcpy(ctx->blocks[block] + off, remaining_data, to_block_end);
        ctx->cursor += to_block_end;
        remaining_data += to_block_end;
        block++;
    }

    size_t remaining_size = size - (remaining_data - (uint8_t*)data);
    for(size_t last_block = BLOCK(ctx->cursor + remaining_size); block != last_block; block++){
        memcpy(ctx->blocks[block], remaining_data, BLOCK_SIZE);
        ctx->cursor += BLOCK_SIZE;
        remaining_data += BLOCK_SIZE;
    }

    remaining_size = size - (remaining_data - (uint8_t*)data);
    if(remaining_size){
        memcpy(ctx->blocks[block], remaining_data, remaining_size);
        ctx->cursor += remaining_size;
        remaining_data += BLOCK_SIZE;
    }

    *written = size;
    adjust_size(ctx);
    return MCE_OK;
}

static MC_Error get_cursor(void *_ctx, size_t *cursor){
    Ctx *ctx = _ctx;
    *cursor = ctx->cursor;
    return MCE_OK;
}

static MC_Error set_cursor(void *_ctx, int64_t cursor, MC_CursorFrom from){
    Ctx *ctx = _ctx;
    switch (from){
    case MC_CURSOR_FROM_BEG: ctx->cursor = cursor; break;
    case MC_CURSOR_FROM_END: ctx->cursor = ctx->size + cursor; break;
    case MC_CURSOR_FROM_CUR: ctx->cursor += cursor; break;
    default: return MCE_INVALID_INPUT;
    }

    if(ctx->cursor < 0){
        ctx->cursor = 0;
    } else if(ctx->cursor > (int64_t)ctx->size){
        ctx->cursor = ctx->size;
    }

    return MCE_OK;
}

static void close(void *_ctx){
    Ctx *ctx = _ctx;

    while(!mc_list_empty(ctx->g)){
        mc_free(ctx->alloc, mc_list_remove(ctx->g));
    }

    if(ctx->blocks){
        mc_free(ctx->alloc, ctx->blocks);
    }
}

static MC_Error alloc_more_blocks(Ctx *ctx, size_t min){
    uint8_t **new_blocks;
    struct Buffer *buffer;

    size_t exp = ctx->blocks_cnt * 2;
    size_t new_cnt = exp > min ? exp : min;

    MC_RETURN_ERROR(mc_alloc_all(ctx->alloc,
        (void**)&new_blocks, sizeof(uint8_t*[new_cnt]),
        (void**)&buffer, sizeof(struct Buffer) + new_cnt * BLOCK_SIZE,
        NULL));

    buffer->next = NULL;
    mc_list_add(&ctx->g, buffer);

    if(ctx->blocks){
        memcpy(new_blocks, ctx->blocks, ctx->blocks_cnt);
        mc_free(ctx->alloc, ctx->blocks);
    }

    ctx->blocks = new_blocks;

    for(size_t block = ctx->blocks_cnt; block != new_cnt; block++){
        ctx->blocks[block] = buffer->data + BLOCK_SIZE * (block - ctx->blocks_cnt);
    }

    ctx->blocks_cnt = new_cnt;

    return MCE_OK;
}

static void adjust_size(Ctx *ctx){
    if((size_t)ctx->cursor > ctx->size){
        ctx->size = ctx->cursor;
    }
}
