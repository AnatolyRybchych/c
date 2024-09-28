#include <mc/os/file.h>

#include <mc/data/string.h>
#include <mc/util/error.h>

#ifdef __linux__
#include "_fd.h"
#endif

#include <errno.h>
#include <malloc.h>

typedef struct FileCtx FileCtx;

struct FileCtx{
#ifdef __linux__
    FdCtx fd_ctx;
#endif
    MC_String *path;
};

static void file_close(void *ctx);

static int open_flags(MC_OpenMode mode);
static int open_permissions(MC_OpenMode mode);
static MC_Error open_fd(MC_Stream **stream, int fd, MC_String *path);

MC_Error mc_fopen(MC_Stream **file, MC_Str path, MC_OpenMode mode){
    int flags = open_flags(mode);
    MC_RETURN_INVALID(flags < 0);

    MC_String *path_string;
    MC_RETURN_ERROR(mc_string(NULL, &path_string, path));

    MC_Error status;
    if(mode & MC_OPEN_CREATE){
        status = open_fd(file, open(path_string->data, open_flags(mode), open_permissions(mode)), path_string);
    }
    else{
        status = open_fd(file, open(path_string->data, open_flags(mode)), path_string);
    }

    if(status == MCE_OK){
        if(mode & MC_OPEN_END){
            mc_set_cursor(*file, 0, MC_CURSOR_FROM_END);
        }
    }

    return status;
}

static int open_flags(MC_OpenMode mode){
    int flags = 0;

    switch (mode & (MC_OPEN_READ | MC_OPEN_WRITE) ){
    case MC_OPEN_READ:
        flags = O_RDONLY;
        break;
    case MC_OPEN_WRITE:
        flags = O_WRONLY;
        break;
    case MC_OPEN_READ | MC_OPEN_WRITE:
        flags = O_RDWR;
        break;
    }

    if(mode & MC_OPEN_CREATE) flags |= O_CREAT;
    if(mode & MC_OPEN_NEW) flags |= O_EXCL;
    if(mode & MC_OPEN_ASYNC) flags |= O_NONBLOCK;
    if(mode & MC_OPEN_CLEAR) flags |= O_TRUNC;
    if(mode & MC_OPEN_EXISTING) flags &= ~O_CREAT;

    return flags;
}

static int open_permissions(MC_OpenMode mode){
    switch (mode & (MC_OPEN_READ | MC_OPEN_WRITE)){
    case MC_OPEN_READ: return 0444;
    case MC_OPEN_WRITE: return 0222;
    case MC_OPEN_READ | MC_OPEN_WRITE: return 0666;
    default: return 0;
    }
}

static MC_Error open_fd(MC_Stream **stream, int fd, MC_String *path){
    if(path == NULL){
        return MCE_OUT_OF_MEMORY;
    }

    FileCtx ctx = {
        .path = path
    };

    MC_Error status = fd_open(&ctx.fd_ctx, fd);
    if(status != MCE_OK){
        mc_free(NULL, ctx.path);
        return status;
    }

    MC_StreamVtab file_vtab = vtbl_fd;
    file_vtab.close = file_close;

    MC_Error error = mc_open(NULL, stream, &file_vtab, sizeof(FileCtx), &ctx);
    if(error != MCE_OK){
        fd_close(&ctx.fd_ctx);
        mc_free(NULL, ctx.path);
    }

    return MCE_OK;
}

MC_Stream *mc_get_stdin(void){
    static MC_String *stdin_string = NULL;
    if(stdin_string == NULL){
        if(mc_string(NULL, &stdin_string, MC_STRC("<stdin>"))){
            return NULL;
        }
    }

    static MC_Stream *stdin_stream = NULL;
    if(stdin_stream == NULL){
        MC_Error error = open_fd(&stdin_stream, STDIN_FILENO, stdin_string);
        if(error){
            return NULL;
        }
    }

    return stdin_stream;
}

MC_Stream *mc_get_stdout(void){
    static MC_String *stdout_string = NULL;
    if(stdout_string == NULL){
        if(mc_string(NULL, &stdout_string, MC_STRC("<stdout>"))){
            return NULL;
        }
    }

    static MC_Stream *stdout_stream = NULL;
    if(stdout_stream == NULL){
        MC_Error error = open_fd(&stdout_stream, STDOUT_FILENO, stdout_string);
        if(error){
            return NULL;
        }
    }

    return stdout_stream;
}

MC_Stream *mc_get_stderr(void){
    static MC_String *stderr_string = NULL;
    if(stderr_string == NULL){
        if(mc_string(NULL, &stderr_string, MC_STRC("<stderr>"))){
            return NULL;
        }
    }

    static MC_Stream *stderr_stream = NULL;
    if(stderr_stream == NULL){
        MC_Error error = open_fd(&stderr_stream, STDERR_FILENO, stderr_string);
        if(error){
            return NULL;
        }
    }

    return stderr_stream;
}

static void file_close(void *ctx){
    FileCtx *file_ctx = ctx;
    fd_close(&file_ctx->fd_ctx);
    mc_free(NULL, file_ctx->path);
}
