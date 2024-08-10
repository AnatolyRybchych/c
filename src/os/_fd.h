#ifndef _FD_H
#define _FD_H

#include <mc/os/file.h>

#include <mc/data/string.h>


#include <sys/file.h>
#include <unistd.h>
#include <limits.h>

typedef struct FdCtx FdCtx;


extern const MC_StreamVtab vtbl_fd;

MC_Error fd_open(FdCtx *ctx, int fd);

MC_Error fd_read(void *ctx, size_t size, void *data, size_t *read);
MC_Error fd_write(void *ctx, size_t size, const void *data, size_t *written);
MC_Error fd_flush(void *ctx);
MC_Error fd_get_cursor(void *ctx, size_t *cursor);
MC_Error fd_set_cursor(void *ctx, size_t cursor);
void fd_close(void *ctx);


struct FdCtx{
    int fd;
};

#endif // 
