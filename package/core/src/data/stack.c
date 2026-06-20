#include <mc/data/stack.h>

#include <mc/util/error.h>
#include <mc/util/minmax.h>
#include <mc/util/memory.h>

#include <stdint.h>
#include <assert.h>
#include <stddef.h>

#define STACK_MIN_BLOCK_BITS 8
#define STACK_MIN_BLOCK_SIZE (1ULL << (STACK_MIN_BLOCK_BITS)) 

#define STACK_MAX_BLOCK_BITS (sizeof(MC_StackChunkSize) * 8)
#define STACK_MAX_BLOCK_SIZE (1ULL << (STACK_MAX_BLOCK_BITS))

#define STACK_GROWTH_SHL 1

static_assert(STACK_MAX_BLOCK_BITS > STACK_MIN_BLOCK_BITS);
static_assert(STACK_GROWTH_SHL > 0);
#define STACK_MAX_GROWTH_LEVELS ((STACK_MAX_BLOCK_BITS - STACK_MIN_BLOCK_BITS) / STACK_GROWTH_SHL)

#define STACK_LAYER_MIN_BITS(LAYER) (STACK_MIN_BLOCK_BITS + (LAYER) * STACK_GROWTH_SHL)
#define STACK_LAYER_MIN_SIZE(LAYER) (1ULL << STACK_LAYER_MIN_BITS(LAYER))

typedef struct Buffer Buffer;

struct Buffer {
    size_t capacity;
    size_t size;
    uint8_t data[];
};

struct MC_Stack {
    MC_Alloc alloc;
    MC_Alloc *base_alloc;
    unsigned buffers_cnt;
    Buffer *buffers[STACK_MAX_GROWTH_LEVELS];
};

static void *stack_alloc(MC_Alloc *this, size_t size);

MC_Error mc_stack_init(MC_Stack **stack, MC_Alloc *base) {
    MC_Stack *res = NULL;
    MC_RETURN_ERROR(mc_alloc(base, sizeof *res, (void**)&res));

    *res = (MC_Stack) {
        .alloc = {
            .alloc = stack_alloc,
            .free = NULL,
        },
        .base_alloc = base
    };

    *stack = res;
    return MCE_OK;
}

void mc_stack_destroy(MC_Stack *stack) {
    mc_stack_settop(stack, 0);
    mc_free(stack->base_alloc, stack);
}

MC_Error mc_stack_reserve(MC_Stack *stack, MC_StackChunkSize size) {
    if (!stack->buffers_cnt) {
        MC_RETURN_ERROR(mc_alloc(stack->base_alloc, sizeof(Buffer) + MAX(size, STACK_MIN_BLOCK_SIZE), (void**)&stack->buffers[0]));
        stack->buffers[0]->capacity = MAX(size, STACK_MIN_BLOCK_SIZE);
        stack->buffers[0]->size = 0;
        stack->buffers_cnt = 1;
        return MCE_OK;
    }

    unsigned top_layer = stack->buffers_cnt - 1;

    unsigned padding = mc_field_padding(stack->buffers[top_layer]->size);
    if (stack->buffers[top_layer]->capacity > stack->buffers[top_layer]->size + padding + size) {
        return MCE_OK;
    }

    size_t new_layer_min_capacity = STACK_LAYER_MIN_SIZE(top_layer + 1);
    size_t new_layer_capacity = MAX(new_layer_min_capacity, size);
    if (new_layer_capacity > STACK_MAX_BLOCK_SIZE) {
        return MCE_OUT_OF_MEMORY;
    }

    Buffer *new_layer = NULL;
    MC_RETURN_ERROR(mc_alloc(stack->base_alloc, sizeof(Buffer) + new_layer_capacity, (void**)&new_layer));

    *new_layer = (Buffer) {
        .capacity = new_layer_capacity,
        .size = 0
    };

    stack->buffers[stack->buffers_cnt++] = new_layer;
    return MCE_OK;
}

size_t mc_stack_top(const MC_Stack *stack) {
    size_t top = 0;

    if (!stack->buffers_cnt) {
        return top;
    }

    for (unsigned layer = 0; layer + 1 < stack->buffers_cnt; layer++) {
        top += stack->buffers[layer]->capacity;
    }

    top += stack->buffers[stack->buffers_cnt - 1]->size;

    return top;
}

MC_Error mc_stack_settop(MC_Stack *stack, size_t top) {
    MC_RETURN_INVALID(stack->buffers_cnt == 0 && top != 0);

    if (stack->buffers_cnt == 0 && top == 0) {
        return MCE_OK;
    }

    unsigned layer = 0;
    size_t offset = top;
    for (layer = 0; layer < stack->buffers_cnt && offset > stack->buffers[layer]->capacity ; layer++) {
        offset -= stack->buffers[layer]->capacity;
    }

    MC_RETURN_INVALID(layer >= stack->buffers_cnt || offset > stack->buffers[layer]->size);

    for (unsigned l = stack->buffers_cnt - 1; l > layer; l--) {
        mc_free(stack->base_alloc, stack->buffers[l]);
        stack->buffers[l] = NULL;
    }

    if (offset == 0) {
        mc_free(stack->base_alloc, stack->buffers[layer]);
        stack->buffers_cnt = layer;
        return MCE_OK;
    }

    stack->buffers[layer]->size = offset;
    stack->buffers_cnt = layer + 1;
    return MCE_OK;
}

MC_Alloc *mc_stack_allocator(MC_Stack *stack) {
    return &stack->alloc;
}

MC_Error mc_stack_alloc(MC_Stack *stack, MC_StackChunkSize size, void **mem) {
    MC_RETURN_ERROR(mc_stack_reserve(stack, size));

    Buffer *buf = stack->buffers[stack->buffers_cnt-1];

    unsigned padding = mc_field_padding(buf->size);

    *mem = &buf->data[buf->size + padding];
    buf->size += padding + size;
    return MCE_OK;
}

static void *stack_alloc(MC_Alloc *this, size_t size) {
    MC_Stack *stack = (MC_Stack*)this;

    void *res = NULL;
    mc_stack_alloc(stack, size, &res);
    return res;
}
