#ifndef MC_DATA_BUFFER_H
#define MC_DATA_BUFFER_H

#include <mc/error.h>

#include <stddef.h>

typedef struct MC_SBuffer MC_SBuffer;

MC_SBuffer mc_sbuffer(size_t capacity, void *buffer);
size_t mc_sbuffer_size(MC_SBuffer *buffer);
size_t mc_sbuffer_avail_size(MC_SBuffer *buffer);
MC_Error mc_sbuffer_write(MC_SBuffer *buffer, size_t size, const void *data);
MC_Error mc_sbuffer_read(MC_SBuffer *buffer, size_t size, void *data);
MC_Error mc_sbuffer_peek(MC_SBuffer *buffer, size_t size, void *data);
void mc_sbuffer_flush(MC_SBuffer *buffer, size_t size);

struct MC_SBuffer {
    size_t buffer_size;
    void *buffer;

    size_t data_size;
    size_t data_offset;
};

#endif // MC_DATA_BUFFER_H
