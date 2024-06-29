#ifndef MC_IO_STREAM_H
#define MC_IO_STREAM_H

#include <mc/error.h>

#include <stdint.h>
#include <stddef.h>

typedef struct MC_Stream MC_Stream;
typedef struct MC_StreamVtab MC_StreamVtab;

MC_Error mc_stream_open(MC_Stream **stream, const MC_StreamVtab *vtab, size_t ctx_size, const void *ctx);
void mc_stream_close(MC_Stream *stream);
void *mc_stream_ctx(MC_Stream *stream);

MC_Error mc_stream_read(MC_Stream *stream, size_t size, void *data, size_t *read);
MC_Error mc_stream_read_all(MC_Stream *stream, size_t size, void *data);
MC_Error mc_stream_write(MC_Stream *stream, size_t size, const void *data, size_t *written);
MC_Error mc_stream_write_all(MC_Stream *stream, size_t size, const void *data);
MC_Error mc_stream_flush(MC_Stream *stream);

MC_Error mc_stream_get_cursor(MC_Stream *stream, size_t *cursor);
MC_Error mc_stream_set_cursor(MC_Stream *stream, size_t cursor);

struct MC_StreamVtab{
    MC_Error (*read)(void *ctx, size_t size, void *data, size_t *read);
    MC_Error (*write)(void *ctx, size_t size, const void *data, size_t *written);
    MC_Error (*flush)(void *ctx);
    MC_Error (*get_cursor)(void *ctx, size_t *cursor);
    MC_Error (*set_cursor)(void *ctx, size_t cursor);
    void (*close)(void *ctx);
};

#endif // MC_IO_STREAM_H
