#include "_stream.h"
#include <malloc.h>
#include <memory.h>

MC_Error mc_stream_open(MC_Stream **stream, const MC_StreamVtab *vtab, size_t ctx_size, const void *ctx){
    MC_Stream *res = malloc(sizeof(MC_Stream) + ctx_size);
    *stream = res;

    if(stream == NULL){
        return MCE_OUT_OF_MEMORY;
    }

    res->vtab = *vtab;
    res->type = STREAM_GENERAL;

    if(ctx){
        memcpy(res->data, ctx, ctx_size);
    }

    return MCE_OK;
}

void mc_stream_close(MC_Stream *stream){
    if(stream == NULL){
        return;
    }

    if(stream->vtab.close){
        stream->vtab.close(stream->data);
    }

    free(stream);
}

void *mc_stream_ctx(MC_Stream *stream){
    if(stream == NULL){
        return NULL;
    }

    return stream->data;
}

MC_Error mc_stream_read(MC_Stream *stream, size_t size, void *data, size_t *read){
    if(stream == NULL || stream->vtab.read == NULL){
        return MCE_NOT_SUPPORTED;
    }

    return stream->vtab.read(stream->data, size, data, read);
}

MC_Error mc_stream_read_all(MC_Stream *stream, size_t size, void *data){
    while(size){
        size_t read;
        MC_Error error = mc_stream_read(stream, size, data, &read);
        if(error != MCE_OK && error != MCE_AGAIN){
            return mc_error_from_errno(error);
        }

        if(size < read){
            return MCE_OVERFLOW;
        }

        data = (uint8_t*)data + read;
        size -= read;
    }

    return MCE_OK;
}

MC_Error mc_stream_write(MC_Stream *stream, size_t size, const void *data, size_t *written){
    if(stream == NULL || stream->vtab.write == NULL){
        return MCE_NOT_SUPPORTED;
    }

    return stream->vtab.write(stream->data, size, data, written);
}

MC_Error mc_stream_write_all(MC_Stream *stream, size_t size, const void *data){
    while(size){
        size_t written;
        MC_Error error = mc_stream_write(stream, size, data, &written);
        if(error != MCE_OK && error != MCE_AGAIN){
            return mc_error_from_errno(error);
        }

        if(size < written){
            return MCE_OVERFLOW;
        }

        data = (uint8_t*)data + written;
        size -= written;
    }

    return MCE_OK;
}

MC_Error mc_stream_flush(MC_Stream *stream){
    if(stream && stream->vtab.flush){
        return stream->vtab.flush(stream->data);
    }

    return MCE_OK;
}

MC_Error mc_stream_get_cursor(MC_Stream *stream, size_t *cursor){
    if(stream == NULL || stream->vtab.get_cursor == NULL){
        return MCE_NOT_SUPPORTED;
    }

    return stream->vtab.get_cursor(stream->data, cursor);
}

MC_Error mc_stream_set_cursor(MC_Stream *stream, size_t cursor){
    if(stream == NULL || stream->vtab.set_cursor == NULL){
        return MCE_NOT_SUPPORTED;
    }

    return stream->vtab.set_cursor(stream->data, cursor);
}
