#include "_stream.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <malloc.h>

static MC_Error fd_read(void *ctx, size_t size, void *data, size_t *read);
static MC_Error fd_write(void *ctx, size_t size, const void *data, size_t *written);
static MC_Error fd_flush(void *ctx);
static MC_Error fd_get_cursor(void *ctx, size_t *cursor);
static MC_Error fd_set_cursor(void *ctx, size_t cursor);
static void fd_close(void *ctx);

static const MC_StreamVtab vtbl_fd = {
    .read = fd_read,
    .write = fd_write,
    .flush = fd_flush,
    .get_cursor = fd_get_cursor,
    .set_cursor = fd_set_cursor,
    .close = fd_close,
};

MC_Error open_fd_stream(MC_Stream **stream, int fd, MC_String *path){
    size_t buffer_size = 0;

    // if (path) {
    //     struct stat stats;
    //     buffer_size = stat(path->data, &stats) ? buffer_size : stats.st_blksize;
    // }

    MC_Error error = mc_open(stream, &vtbl_fd, sizeof(FileCtx) + buffer_size, NULL);
    if(error){
        return error;
    }

    FileCtx *ctx = mc_ctx(*stream);
    ctx->fd = fd;
    ctx->path = path;
    ctx->buffer_size = buffer_size;
    
    return MCE_OK;
}

static MC_Error fd_read(void *ctx, size_t size, void *data, size_t *read_size){
    FileCtx *file = ctx;

    ssize_t sz_read = read(file->fd, data, size);
    if(sz_read < 0){
        return mc_error_from_errno(errno);
    }

    *read_size = sz_read;
    return MCE_OK;
}

static MC_Error fd_write(void *ctx, size_t size, const void *data, size_t *written){
    FileCtx *file = ctx;

    ssize_t sz_written = write(file->fd, data, size);
    if(sz_written < 0){
        return mc_error_from_errno(errno);
    }

    *written = sz_written;
    return MCE_OK;
}

static MC_Error fd_flush(void *ctx){
    (void)ctx;
    return MCE_OK;
}

static MC_Error fd_get_cursor(void *ctx, size_t *cursor){
    FileCtx *file = ctx;
    off_t pos = lseek(file->fd, 0, SEEK_CUR);
    if (pos == (off_t)-1) {
        return mc_error_from_errno(errno);
    }

    *cursor = pos;
    return MCE_OK;
}

static MC_Error fd_set_cursor(void *ctx, size_t cursor){
    FileCtx *file = ctx;
    off_t pos = lseek(file->fd, cursor, SEEK_SET);
    if (pos == (off_t)-1) {
        return mc_error_from_errno(errno);
    }

    return MCE_OK;
}

static void fd_close(void *ctx){
    FileCtx *file = ctx;
    if(file->close && file->fd >= 0){
        close(file->fd);
    }

    if(file->path) free(file->path);
}
