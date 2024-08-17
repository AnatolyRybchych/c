
#ifdef __linux__

#include "_fd.h"

#include <errno.h>

const MC_StreamVtab vtbl_fd = {
    .read = fd_read,
    .write = fd_write,
    .flush = fd_flush,
    .get_cursor = fd_get_cursor,
    .set_cursor = fd_set_cursor,
    .close = fd_close,
};

MC_Error fd_open(FdCtx *ctx, int fd){
    if(fd < 0){
        return mc_error_from_errno(errno);
    }

    ctx->fd = fd;
    return MCE_OK;
}

MC_Error fd_read(void *ctx, size_t size, void *data, size_t *read_size){
    FdCtx *fd_ctx = ctx;

    ssize_t sz_read = read(fd_ctx->fd, data, size);
    if(sz_read < 0){
        *read_size = 0;
        return mc_error_from_errno(errno);
    }

    *read_size = sz_read;
    return MCE_OK;
}

MC_Error fd_write(void *ctx, size_t size, const void *data, size_t *written){
    FdCtx *fd_ctx = ctx;

    ssize_t sz_written = write(fd_ctx->fd, data, size);
    if(sz_written < 0){
        *written = 0;
        return mc_error_from_errno(errno);
    }

    *written = sz_written;
    return MCE_OK;
}

MC_Error fd_flush(void *ctx){
    (void)ctx;
    return MCE_OK;
}

MC_Error fd_get_cursor(void *ctx, size_t *cursor){
    FdCtx *fd_ctx = ctx;
    off_t pos = lseek(fd_ctx->fd, 0, SEEK_CUR);
    if (pos == (off_t)-1) {
        return mc_error_from_errno(errno);
    }

    *cursor = pos;
    return MCE_OK;
}

MC_Error fd_set_cursor(void *ctx, size_t cursor, MC_CursorFrom from){
    FdCtx *fd_ctx = ctx;

    int seek = from == MC_CURSOR_FROM_BEG
        ? SEEK_SET
        : from == MC_CURSOR_FROM_END
            ? SEEK_END
            : SEEK_CUR;

    off_t pos = lseek(fd_ctx->fd, cursor, seek);
    if (pos == (off_t)-1) {
        return mc_error_from_errno(errno);
    }

    return MCE_OK;
}

void fd_close(void *ctx){
    FdCtx *fd_ctx = ctx;
    close(fd_ctx->fd);
}

#endif
