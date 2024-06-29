#ifndef _STREAM_H
#define _STREAM_H

#include <mc/io/stream.h>
#include <mc/data/str.h>
#include <mc/data/string.h>

#include <unistd.h>
#include <errno.h>
#include <stdbool.h>

typedef struct FileCtx FileCtx;

typedef unsigned StreamType;
enum StreamType{
    STREAM_GENERAL,
    STREAM_FILE,
};

struct MC_Stream{
    MC_StreamVtab vtab;
    StreamType type;
    uint8_t data[];
};

struct FileCtx{
    int fd;
    bool close;
    MC_String *path;
    size_t buffer_size; 
    uint8_t buffer[];
};

MC_Error open_fd_stream(MC_Stream **stream, int fd, MC_String *path);

#endif // _STREAM_H
