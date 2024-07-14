#include <mc/io/file.h>
#include "_stream.h"

#include <sys/file.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <malloc.h>

static int open_flags(MC_OpenMode mode);

MC_Error mc_fopen(MC_Stream **file, MC_Str path, MC_OpenMode mode){
    int flags = open_flags(mode);
    if(flags < 0){
        return MCE_INVALID_INPUT;
    }

    MC_String *path_string = mc_string(path);
    if(path_string == NULL){
        return MCE_OUT_OF_MEMORY;
    }

    int fd = open(path_string->data, flags);
    if(fd < 0){
        free(path_string);
        return mc_error_from_errno(errno);
    }

    MC_Error error = open_fd_stream(file, fd, path_string);
    if(error){
        close(fd);
        free(path_string);
    }

    return error;
}

static int open_flags(MC_OpenMode mode){
    switch (mode){
    case MC_OPEN_READ: return O_RDONLY;
    case MC_OPEN_WRITE: return O_RDWR | O_CREAT;
    case MC_OPEN_APPEND: return O_RDWR | O_APPEND | O_CREAT;
    default: return -1;
    }
}

MC_Stream *mc_get_stdin(void){
    static MC_String *stdin_string = NULL;
    if(stdin_string == NULL){
        stdin_string = mc_string(MC_STRC("<stdin>"));
        if(stdin_string == NULL){
            return NULL;
        }
    }

    static MC_Stream *stdin_stream = NULL;
    if(stdin_stream == NULL){
        MC_Error error = open_fd_stream(&stdin_stream, STDIN_FILENO, stdin_string);
        if(error){
            return NULL;
        }
    }

    return stdin_stream;
}

MC_Stream *mc_get_stdout(void){
    static MC_String *stdout_string = NULL;
    if(stdout_string == NULL){
        stdout_string = mc_string(MC_STRC("<stdout>"));
        if(stdout_string == NULL){
            return NULL;
        }
    }

    static MC_Stream *stdout_stream = NULL;
    if(stdout_stream == NULL){
        MC_Error error = open_fd_stream(&stdout_stream, STDOUT_FILENO, stdout_string);
        if(error){
            return NULL;
        }
    }

    return stdout_stream;
}

MC_Stream *mc_get_stderr(void){
    static MC_String *stderr_string = NULL;
    if(stderr_string == NULL){
        stderr_string = mc_string(MC_STRC("<stdout>"));
        if(stderr_string == NULL){
            return NULL;
        }
    }

    static MC_Stream *stderr_stream = NULL;
    if(stderr_stream == NULL){
        MC_Error error = open_fd_stream(&stderr_stream, STDERR_FILENO, stderr_string);
        if(error){
            return NULL;
        }
    }

    return stderr_stream;
}

