#include "_stream.h"

#include <mc/data/struct.h>

#include <malloc.h>
#include <memory.h>

MC_Error mc_open(MC_Stream **stream, const MC_StreamVtab *vtab, size_t ctx_size, const void *ctx){
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

void mc_close(MC_Stream *stream){
    if(stream == NULL){
        return;
    }

    if(stream->vtab.close){
        stream->vtab.close(stream->data);
    }

    free(stream);
}

void *mc_ctx(MC_Stream *stream){
    if(stream == NULL){
        return NULL;
    }

    return stream->data;
}

MC_Error mc_read(MC_Stream *stream, size_t size, void *data, size_t *read){
    if(stream == NULL || stream->vtab.read == NULL){
        return MCE_NOT_SUPPORTED;
    }

    return stream->vtab.read(stream->data, size, data, read);
}

MC_Error mc_read_all(MC_Stream *stream, size_t size, void *data){
    while(size){
        size_t read;
        MC_Error error = mc_read(stream, size, data, &read);
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

MC_Error mc_write(MC_Stream *stream, size_t size, const void *data, size_t *written){
    if(stream == NULL || stream->vtab.write == NULL){
        return MCE_NOT_SUPPORTED;
    }

    return stream->vtab.write(stream->data, size, data, written);
}

MC_Error mc_write_all(MC_Stream *stream, size_t size, const void *data){
    while(size){
        size_t written;
        MC_Error error = mc_write(stream, size, data, &written);
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

MC_Error mc_fmtv(MC_Stream *stream, const char *fmt, va_list args){
    MC_Error status;

    va_list args_cp;
    va_copy(args_cp, args);
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args_cp);

    if(len < 512){
        char buffer[len + 1];
        vsnprintf(buffer, sizeof(buffer), fmt, args_cp);
        status = mc_write_all(stream, len, buffer);
    }
    else{
        MC_String *buffer = mc_string_fmtv(fmt, args);
        if(buffer){
            status = mc_write_all(stream, buffer->len, buffer->data);
            free(buffer);
        }
        else{
            status = MCE_OUT_OF_MEMORY;
        }
    }
    
    return status;
}

MC_Error mc_fmt(MC_Stream *stream, const char *fmt, ...){
    va_list args;
    va_start(args, fmt);
    MC_Error error = mc_fmtv(stream, fmt, args);
    va_end(args);
    return error;
}

MC_Error mc_packv(MC_Stream *stream, const char *fmt, va_list args){
    int size = mc_struct_calcsize(fmt);
    MC_RETURN_INVALID(size < 0);

    if(size <= 512){
        char buffer[size];
        mc_struct_vnpack(buffer, ~(unsigned)0, fmt, args);
        return mc_write_all(stream, size, buffer);
    }

    char *buffer = malloc(size);
    if(buffer == NULL){
        return MCE_OUT_OF_MEMORY;
    }

    mc_struct_vnpack(buffer, ~(unsigned)0, fmt, args);
    MC_Error status = mc_write_all(stream, size, buffer);
    free(buffer);
    return status;
}

MC_Error mc_pack(MC_Stream *stream, const char *fmt, ...){
    va_list args;
    va_start(args, fmt);
    size_t res = mc_packv(stream, fmt, args);
    va_end(args);
    return res;
}

MC_Error mc_unpackv(MC_Stream *stream, const char *fmt, va_list args){
    int size = mc_struct_calcsize(fmt);
    MC_RETURN_INVALID(size < 0);

    if(size <= 512){
        char buffer[size];
        MC_RETURN_ERROR(mc_read_all(stream, size, buffer));
        mc_struct_vnunpack(buffer, ~(unsigned)0, fmt, args);
        return MCE_OK;
    }

    char *buffer = malloc(size);
    if(buffer == NULL){
        return MCE_OUT_OF_MEMORY;
    }

    MC_Error status = mc_read_all(stream, size, buffer);
    if(status != MCE_OK){
        free(buffer);
        return status;
    }

    mc_struct_vnunpack(buffer, ~(unsigned)0, fmt, args);
    free(buffer);
    return MCE_OK;
}

MC_Error mc_unpack(MC_Stream *stream, const char *fmt, ...){
    va_list args;
    va_start(args, fmt);
    size_t res = mc_unpackv(stream, fmt, args);
    va_end(args);
    return res;
}


MC_Error mc_flush(MC_Stream *stream){
    if(stream && stream->vtab.flush){
        return stream->vtab.flush(stream->data);
    }

    return MCE_OK;
}

MC_Error mc_get_cursor(MC_Stream *stream, size_t *cursor){
    if(stream == NULL || stream->vtab.get_cursor == NULL){
        return MCE_NOT_SUPPORTED;
    }

    return stream->vtab.get_cursor(stream->data, cursor);
}

MC_Error mc_set_cursor(MC_Stream *stream, size_t cursor){
    if(stream == NULL || stream->vtab.set_cursor == NULL){
        return MCE_NOT_SUPPORTED;
    }

    return stream->vtab.set_cursor(stream->data, cursor);
}
