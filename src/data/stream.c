#include <mc/data/stream.h>

#include <mc/data/struct.h>
#include <mc/data/string.h>
#include <mc/data/str.h>
#include <mc/util/error.h>
#include <mc/util/assert.h>
#include <mc/util/util.h>

#include <malloc.h>
#include <memory.h>

struct MC_Stream{
    MC_Alloc *alloc;
    MC_StreamVtab vtab;
    uint8_t data[];
};

MC_Error mc_open(MC_Alloc *alloc, MC_Stream **stream, const MC_StreamVtab *vtab, size_t ctx_size, const void *ctx){
    MC_RETURN_ERROR(mc_alloc(alloc, sizeof(MC_Stream) + ctx_size, (void**)stream));
    MC_Stream *res = *stream;

    res->vtab = *vtab;
    res->alloc = alloc;

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

    mc_free(stream->alloc, stream);
}

void *mc_ctx(MC_Stream *stream){
    if(stream == NULL){
        return NULL;
    }

    return stream->data;
}

MC_Error mc_read_async(MC_Stream *stream, size_t size, void *data, size_t *read){
    if(stream == NULL) {
        MC_OPTIONAL_SET(read, 0);
        return MCE_OK;
    }

    if(stream->vtab.read == NULL){
        return MCE_NOT_SUPPORTED;
    }

    size_t dummy;
    return stream->vtab.read(stream->data, size, data, read ? read : &dummy);
}

MC_Error mc_read(MC_Stream *stream, size_t size, void *data, size_t *ret_read){
    if(stream == NULL) {
        MC_OPTIONAL_SET(ret_read, 0);
        return MCE_OK;
    }

    size_t read = 0;

    while(true){
        size_t cur_read;
        MC_Error error = mc_read_async(stream, size, data, &cur_read);
        MC_ASSERT_BUG(size >= cur_read);

        read += cur_read;
        size -= cur_read;

        if(error == MCE_AGAIN){
            continue;
        }

        MC_OPTIONAL_SET(ret_read, read);
        return error;
    }
}

MC_Error mc_write_async(MC_Stream *stream, size_t size, const void *data, size_t *written){
    if(stream == NULL) {
        MC_OPTIONAL_SET(written, 0);
        return MCE_OK;
    }

    if(stream->vtab.write == NULL){
        return MCE_NOT_SUPPORTED;
    }

    size_t dummy;
    return stream->vtab.write(stream->data, size, data, written ? written : &dummy);
}

MC_Error mc_write(MC_Stream *stream, size_t size, const void *data, size_t *ret_written){
    if (stream == NULL) {
        MC_OPTIONAL_SET(ret_written, size);
        return MCE_OK;
    }

    size_t written = 0;

    while(true){
        size_t cur_written;
        MC_Error error = mc_write_async(stream, size, data, &cur_written);
        MC_ASSERT_BUG(size <= cur_written);

        written += cur_written;
        size -= cur_written;

        if(error == MCE_AGAIN){
            continue;
        }

        MC_OPTIONAL_SET(ret_written, written);
        return error;
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
        status = mc_write(stream, len, buffer, NULL);
    }
    else{
        MC_String *buffer;
        MC_RETURN_ERROR(mc_string_fmtv(NULL, &buffer, fmt, args));
        status = mc_write(stream, buffer->len, buffer->data, NULL);
        mc_free(NULL, buffer);
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
        return mc_write(stream, size, buffer, NULL);
    }

    char *buffer;
    MC_RETURN_ERROR(mc_alloc(NULL, size, (void**)&buffer));

    mc_struct_vnpack(buffer, ~(unsigned)0, fmt, args);
    MC_Error status = mc_write(stream, size, buffer, NULL);
    mc_free(NULL, buffer);
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
        MC_RETURN_ERROR(mc_read(stream, size, buffer, NULL));
        mc_struct_vnunpack(buffer, ~(unsigned)0, fmt, args);
        return MCE_OK;
    }

    char *buffer;
    MC_RETURN_ERROR(mc_alloc(NULL, size, (void**)&buffer));

    MC_Error status = mc_read(stream, size, buffer, NULL);
    if(status != MCE_OK){
        mc_free(NULL, buffer);
        return status;
    }

    mc_struct_vnunpack(buffer, ~(unsigned)0, fmt, args);
    mc_free(NULL, buffer);
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

MC_Error mc_set_cursor(MC_Stream *stream, size_t cursor, MC_CursorFrom from){
    if(stream == NULL || stream->vtab.set_cursor == NULL){
        return MCE_NOT_SUPPORTED;
    }

    return stream->vtab.set_cursor(stream->data, cursor, from);
}
