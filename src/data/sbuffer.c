#include <mc/data/sbuffer.h>
#include <mc/util/error.h>

#include <memory.h>
#include <stdint.h>

MC_SBuffer mc_sbuffer(size_t capacity, void *buffer) {
    return (MC_SBuffer) {
        .buffer = buffer,
        .buffer_size = capacity
    };
}

size_t mc_sbuffer_size(MC_SBuffer *buffer) {
    return buffer->data_size;
}

size_t mc_sbuffer_avail_size(MC_SBuffer *buffer) {
    return buffer->buffer_size - buffer->data_size;
}

MC_Error mc_sbuffer_write(MC_SBuffer *buffer, size_t size, const void *data) {
    mc_fmt(MC_STDERR, "write:\n------------\n%.*s\n------------\n", size, data);
    size_t avail_size = mc_sbuffer_avail_size(buffer);
    MC_Error status = MCE_OK;
    if(size > avail_size) {
        status = MCE_OVERFLOW;
        size = avail_size;
    }

    size_t data_end_offset = buffer->data_offset + buffer->data_size;
    if(data_end_offset + size > buffer->buffer_size) {
        size_t size_to_edge = buffer->buffer_size - data_end_offset;
        memcpy((uint8_t*)buffer->buffer + data_end_offset, data, size_to_edge);
        memcpy(buffer->buffer, (uint8_t*)data + size_to_edge, size - size_to_edge);
    }
    else {
        memcpy((uint8_t*)buffer->buffer + data_end_offset, data, size);
    }

    buffer->data_size += size;
    return status;
}

MC_Error mc_sbuffer_read(MC_SBuffer *buffer, size_t size, void *data) {
    MC_Error peek_status = mc_sbuffer_peek(buffer, size, data);
    mc_sbuffer_flush(buffer, size);
    MC_RETURN_ERROR(peek_status);
    return MCE_OK;
}

MC_Error mc_sbuffer_peek(MC_SBuffer *buffer, size_t size, void *data) {
    MC_Error status;
    if(size > buffer->data_size) {
        status = MCE_TRUNCATED;
        size = buffer->data_size;
    }

    if(buffer->data_offset + size > buffer->buffer_size) {
        size_t size_to_edge = buffer->buffer_size - buffer->data_offset;
        memcpy(data, (uint8_t*)buffer->buffer + buffer->data_offset, size_to_edge);
        memcpy((uint8_t*)data + size_to_edge, buffer->buffer, size - size_to_edge);
    }
    else {
        memcpy(data, (uint8_t*)buffer->buffer + buffer->data_offset, size);
    }

    mc_fmt(MC_STDERR, "peek:\n------------\n%.*s\n------------\n", size, data);

    return status;
}

void mc_sbuffer_flush(MC_SBuffer *buffer, size_t size) {
    mc_fmt(MC_STDERR, "flush: %zu\n", size);
    if(size >= buffer->data_size) {
        buffer->data_size = 0;
        buffer->data_offset = 0;
    }
    else{
        buffer->data_size -= size;
        buffer->data_offset += size;
        if(buffer->data_offset > buffer->buffer_size) {
            buffer->data_offset -= buffer->buffer_size;
        }
    }
}
