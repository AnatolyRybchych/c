#include <mc/net/proto/http/writer.h>

#include <mc/data/arena.h>
#include <mc/data/hmap.h>
#include <mc/data/string.h>
#include <mc/util/error.h>

struct MC_HttpWriter {
    MC_Alloc *alloc;
    MC_Stream *dst;
    const MC_HttpMessage *msg;
};

MC_Error mc_http_writer_new(MC_Alloc *alloc, MC_HttpWriter **writer) {
    MC_HttpWriter *new;
    MC_RETURN_ERROR(mc_alloc(alloc, sizeof(MC_HttpWriter), (void**)&new));

    *new = (MC_HttpWriter) {
        .alloc = alloc,
    };

    *writer = new;
    return MCE_OK;
}

void mc_http_writer_delete(MC_HttpWriter *writer) {
    mc_free(writer->alloc, writer);
}

MC_Error mc_http_writer_beg(MC_HttpWriter *writer, MC_Stream *destination, const MC_HttpMessage *message) {
    MC_RETURN_INVALID(writer->msg != NULL);
    writer->dst = destination;
    writer->msg = message;

    return MCE_OK;
}

MC_Error mc_http_writer_write(MC_HttpWriter *writer) {
    MC_RETURN_INVALID(writer->msg == NULL);

    if(mc_http_is_request(writer->msg)) {
        MC_HttpMethod method;
        MC_RETURN_ERROR(mc_http_get_method(writer->msg, &method));

        const char *method_str = mc_http_method_str(method);
        MC_RETURN_INVALID(method_str == NULL);

        MC_Str path;
        MC_RETURN_ERROR(mc_http_get_path(writer->msg, &path));

        MC_HttpVersion version = mc_http_get_version(writer->msg);

        MC_RETURN_ERROR(mc_fmt(writer->dst, "%s %.*s HTTP/%u.%u\r\n",
            method_str, mc_str_len(path), path.beg, (unsigned)version.major, (unsigned)version.minor));
    }
    else if(mc_http_is_response(writer->msg)) {
        unsigned code;
        MC_RETURN_ERROR(mc_http_get_code(writer->msg, &code));

        const char *status = mc_http_code_status(code);
        MC_RETURN_INVALID(status == NULL);

        MC_HttpVersion version = mc_http_get_version(writer->msg);

        MC_RETURN_ERROR(mc_fmt(writer->dst, "HTTP/%u.%u %u %s\r\n",
            (unsigned)version.major, (unsigned)version.minor, code, status));
    }
    else {
        return MCE_INVALID_INPUT;
    }

    MC_HMap *headers = mc_http_headers((MC_HttpMessage*)writer->msg);
    for(MC_HMapIterator iter = mc_hmap_iter(headers); mc_hmap_iter_next(&iter);) {
        MC_Str value = mc_string_str(iter.value);
        MC_RETURN_ERROR(mc_fmt(writer->dst, "%.*s: %.*s\r\n",
            mc_str_len(iter.key), iter.key.beg, mc_str_len(value), value.beg));
    }

    MC_RETURN_ERROR(mc_fmt(writer->dst, "\r\n"));

    writer->dst = NULL;
    writer->msg = NULL;
    return MCE_OK;
}
