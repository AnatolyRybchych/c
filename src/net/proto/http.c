#include <mc/net/proto/http.h>

#include <mc/data/arena.h>
#include <mc/util/error.h>
#include <mc/util/assert.h>
#include <mc/data/sbuffer.h>
#include <mc/data/mstream.h>
#include <mc/data/encoding/url.h>
#include <mc/data/list.h>
#include <mc/util/util.h>

typedef unsigned ReaderState;
enum ReaderState {
    READERF_INCOMPLETE      = 1 << 7,

    READER_ERROR            = 0,
    READER_BEGIN            = 1,
    READER_DONE             = 2,
    READER_FIRST_LINE       = 3 | READERF_INCOMPLETE,
    READER_HEADER_LINE      = 4 | READERF_INCOMPLETE,
    READER_BODY             = 5 | READERF_INCOMPLETE,
    READER_BODY_SIZED       = 6 | READERF_INCOMPLETE,
};

typedef unsigned MC_HttpMessageType;
enum MC_HttpMessageType {
    MESSAGE_UNKNOWN,
    MESSAGE_REQUEST,
    MESSAGE_RESPONSE,
};

struct MC_HttpMessage {
    MC_HttpVersion version;
    MC_HttpMessageType type;
    union {
        struct {
            MC_HttpMethod method;
            MC_String *path;
        } request;

        struct {
            unsigned code;
            MC_String *status;
        } response;
    };

    // MC_Str: MC_String
    MC_HMap *headers_map;
    MC_HttpHeader *headers, *last_header;
    MC_Stream *body_stream;
};

struct MC_HttpComposer {
    MC_Alloc *alloc;

    MC_HttpMessage *message;
};

struct MC_HttpReader {
    ReaderState state;
    MC_Alloc *alloc;
    MC_Stream *source;
    MC_Alloc *msg_alloc;
    size_t bufsz;
    char buf[256];

    MC_Arena *msg_arena;
    MC_Stream *line;
    MC_HttpMessage *message;
    size_t body_size;
    size_t body_remaining;
};

struct MC_HttpWriter {
    MC_Alloc *alloc;
    MC_Stream *dst;
    const MC_HttpMessage *msg;
};

static MC_Error read_sized(MC_HttpReader *reader, MC_Stream *dst, size_t size, size_t *remaining);
static MC_Error read_line(MC_HttpReader *reader, size_t max_size);
static MC_Error reset_line(MC_HttpReader *reader);
static MC_Error reader_begin(MC_HttpReader *reader);
static MC_Error read_first_line(MC_HttpReader *reader);
static MC_Error read_header_line(MC_HttpReader *reader);
static MC_Error read_body_sized(MC_HttpReader *reader, MC_Stream *body);
static MC_Error read_body(MC_HttpReader *reader, MC_Stream *body);
static MC_Error parse_http_version(MC_Str src, MC_HttpVersion *version);
static MC_Error reader_read(MC_HttpReader *reader, MC_Stream *body);

MC_Error mc_http_reader_new(MC_Alloc *alloc, MC_HttpReader **out_reader, MC_Stream *source) {
    *out_reader = NULL;

    MC_HttpReader *reader;
    MC_RETURN_ERROR(mc_alloc(alloc, sizeof(MC_HttpReader), (void**)&reader));

    MC_Arena *msg_arena;
    MC_Error error = mc_arena_init(&msg_arena, alloc);
    if(error != MCE_OK) {
        mc_free(alloc, reader);
        return error;
    }

    *reader = (MC_HttpReader) {
        .alloc = alloc,
        .msg_arena = msg_arena,
        .source = source,
        .state = READER_BEGIN,
    };
    reader->msg_alloc = mc_arena_allocator(reader->msg_arena);

    *out_reader = reader;
    return MCE_OK;
}


void mc_http_reader_delete(MC_HttpReader *reader) {
    if(reader == NULL) {
        return;
    }

    mc_arena_destroy(reader->msg_arena);
    mc_free(reader->alloc, reader);
}

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

    if(writer->msg->type == MESSAGE_REQUEST) {
        const char *method = mc_http_method_str(writer->msg->request.method);
        MC_RETURN_INVALID(method == NULL);
        MC_Str path = mc_string_str(writer->msg->request.path);
        MC_RETURN_INVALID(mc_str_empty(path));
        MC_HttpVersion version = writer->msg->version;

        MC_RETURN_ERROR(mc_fmt(writer->dst, "%s %.*s HTTP/%u.%u\r\n",
            method, mc_str_len(path), path.beg, (unsigned)version.major, (unsigned)version.minor));
    }
    else {
        unsigned code = writer->msg->response.code;
        MC_Str status = mc_string_str(writer->msg->response.status);
        if(mc_str_empty(status)) {
            const char *s = mc_http_code_status(code);
            status = mc_strc(s);
        }

        MC_HttpVersion version = writer->msg->version;

        MC_RETURN_ERROR(mc_fmt(writer->dst, "HTTP/%u.%u %u %.*s\r\n",
            (unsigned)version.major, (unsigned)version.minor, code, mc_str_len(status), status.beg));
    }

    MC_LIST_FOR(MC_HttpHeader, (void*)writer->msg->headers, hdr) {
        MC_Str key = hdr->key;
        MC_RETURN_ERROR(mc_fmt(writer->dst, "%.*s: %.*s\r\n",
            mc_str_len(key), key.beg, mc_str_len(hdr->value), hdr->value.beg));
    }

    MC_RETURN_ERROR(mc_fmt(writer->dst, "\r\n"));

    writer->dst = NULL;
    writer->msg = NULL;
    return MCE_OK;
}

const char *mc_http_code_status(unsigned code) {
	switch (code){
	//####### 1xx - Informational #######
	case 100: return "Continue";
	case 101: return "Switching Protocols";
	case 102: return "Processing";
	case 103: return "Early Hints";

	//####### 2xx - Successful #######
	case 200: return "OK";
	case 201: return "Created";
	case 202: return "Accepted";
	case 203: return "Non-Authoritative Information";
	case 204: return "No Content";
	case 205: return "Reset Content";
	case 206: return "Partial Content";
	case 207: return "Multi-Status";
	case 208: return "Already Reported";
	case 226: return "IM Used";

	//####### 3xx - Redirection #######
	case 300: return "Multiple Choices";
	case 301: return "Moved Permanently";
	case 302: return "Found";
	case 303: return "See Other";
	case 304: return "Not Modified";
	case 305: return "Use Proxy";
	case 307: return "Temporary Redirect";
	case 308: return "Permanent Redirect";

	//####### 4xx - Client Error #######
	case 400: return "Bad Request";
	case 401: return "Unauthorized";
	case 402: return "Payment Required";
	case 403: return "Forbidden";
	case 404: return "Not Found";
	case 405: return "Method Not Allowed";
	case 406: return "Not Acceptable";
	case 407: return "Proxy Authentication Required";
	case 408: return "Request Timeout";
	case 409: return "Conflict";
	case 410: return "Gone";
	case 411: return "Length Required";
	case 412: return "Precondition Failed";
	case 413: return "Content Too Large";
	case 414: return "URI Too Long";
	case 415: return "Unsupported Media Type";
	case 416: return "Range Not Satisfiable";
	case 417: return "Expectation Failed";
	case 418: return "I'm a teapot";
	case 421: return "Misdirected Request";
	case 422: return "Unprocessable Content";
	case 423: return "Locked";
	case 424: return "Failed Dependency";
	case 425: return "Too Early";
	case 426: return "Upgrade Required";
	case 428: return "Precondition Required";
	case 429: return "Too Many Requests";
	case 431: return "Request Header Fields Too Large";
	case 451: return "Unavailable For Legal Reasons";

	//####### 5xx - Server Error #######
	case 500: return "Internal Server Error";
	case 501: return "Not Implemented";
	case 502: return "Bad Gateway";
	case 503: return "Service Unavailable";
	case 504: return "Gateway Timeout";
	case 505: return "HTTP Version Not Supported";
	case 506: return "Variant Also Negotiates";
	case 507: return "Insufficient Storage";
	case 508: return "Loop Detected";
	case 510: return "Not Extended";
	case 511: return "Network Authentication Required";
	default: return NULL;
    }
}

MC_Error mc_http_reader_read(MC_HttpReader *reader, const MC_HttpMessage **message, MC_Stream *body) {
    while (reader->state != READER_DONE) {
        MC_Error status = reader_read(reader, body);
        if(status != MCE_OK) {
            if(status != MCE_AGAIN) {
                reader->state = READER_ERROR;
            }

            return status;
        }
    }

    MC_OPTIONAL_SET(message, reader->message);
    return MCE_OK;
}

MC_Error mc_http_reader_finish(MC_HttpReader *reader) {
    while(reader->state & READERF_INCOMPLETE ) {
        MC_RETURN_ERROR(mc_http_reader_read(reader, NULL, NULL));
    }

    mc_arena_reset(reader->msg_arena);
    MC_RETURN_INVALID(reader->state == READER_ERROR);

    reader->state = READER_BEGIN;
    reader->message = NULL;
    reader->line = NULL;
    return MCE_OK;
}

const char *mc_http_method_str(MC_HttpMethod method) {
    const char *result = NULL;
    MC_ENUM_TO_STRING(result, method, MC_ITER_HTTP_METHODS, MC_HTTP_)
    return result;
}

MC_HttpMethod mc_http_method_from_str(MC_Str str) {
    MC_HttpMethod result;
    MC_ENUM_FROM_STR(result, str, MC_ITER_HTTP_METHODS, MC_HTTP_){
        return MC_HTTP_UNDEFINED;
    };
    return result;
}

MC_Error mc_http_composer_new(MC_Alloc *alloc, MC_HttpComposer **composer) {
    *composer = NULL;
    MC_HttpComposer *new;
    MC_RETURN_ERROR(mc_alloc(alloc, sizeof(MC_HttpComposer), (void**)&new));
    *new = (MC_HttpComposer) {
        .alloc = alloc,
        .message = NULL
    };

    *composer = new;
    return MCE_OK;
}

void mc_http_composer_delete(MC_HttpComposer *composer) {
    if(composer->message) {
        mc_free(composer->alloc, composer->message);
    }

    mc_free(composer->alloc, composer);
}

MC_Error mc_http_composer_get_message(MC_HttpComposer *composer, const MC_HttpMessage **message) {
    *message = composer->message;
    MC_RETURN_INVALID(composer->message == NULL);
    return MCE_OK;
}

MC_Error mc_http_composer_begin_request(MC_HttpComposer *composer) {
    MC_RETURN_INVALID(composer->message != NULL);
    MC_RETURN_ERROR(mc_alloc(composer->alloc, sizeof(MC_HttpMessage), (void**)&composer->message));
    *composer->message = (MC_HttpMessage) {
        .type = MESSAGE_REQUEST,
        .version = {.major = 1, .minor = 1},
        .request = {
            .method = MC_HTTP_GET,
        },
    };

    return MCE_OK;
}

MC_Error mc_http_composer_begin_response(MC_HttpComposer *composer) {
    MC_RETURN_INVALID(composer->message != NULL);
    MC_RETURN_ERROR(mc_alloc(composer->alloc, sizeof(MC_HttpMessage), (void**)&composer->message));
    *composer->message = (MC_HttpMessage) {
        .type = MESSAGE_RESPONSE,
        .version = {.major = 1, .minor = 1},
        .response = {
            .code = 200,
        },
    };

    return MCE_OK;
}

MC_Error mc_http_composer_set_status_code(MC_HttpComposer *composer, int code) {
    MC_RETURN_INVALID(composer->message == NULL);
    MC_RETURN_INVALID(composer->message->type != MESSAGE_RESPONSE);

    composer->message->response.code = code;
    return MCE_OK;
}

MC_Error mc_http_composer_set_method(MC_HttpComposer *composer, MC_HttpMethod method) {
    MC_RETURN_INVALID(composer->message == NULL);
    MC_RETURN_INVALID(composer->message->type != MESSAGE_REQUEST);
    
    composer->message->request.method = method;
    return MCE_OK;
}

MC_Error mc_http_composer_set_path(MC_HttpComposer *composer, MC_Str path) {
    MC_RETURN_INVALID(composer->message == NULL);
    MC_RETURN_INVALID(composer->message->type != MESSAGE_REQUEST);
    
    MC_String *new_path;
    MC_RETURN_ERROR(mc_string(composer->alloc, &new_path, path));
    if(composer->message->request.path) {
        mc_free(composer->alloc, composer->message->request.path);
    }
    composer->message->request.path = new_path;

    return MCE_OK;
}

MC_Error mc_http_composer_set_version(MC_HttpComposer *composer, MC_HttpVersion version) {
    MC_RETURN_INVALID(composer->message == NULL);
    composer->message->version = version;
    return MCE_OK;
}

MC_Error mc_http_composer_set_headers(MC_HttpComposer *composer, const MC_HttpHeader *headers) {
    MC_RETURN_INVALID(composer->message == NULL);

    MC_LIST_FOR(MC_HttpHeader, (void*)headers, hdr) {
        MC_RETURN_ERROR(mc_http_composer_set_header(composer, hdr->key, hdr->value));
    }

    return MCE_OK;
}

MC_Error mc_http_composer_set_header(MC_HttpComposer *composer, MC_Str key, MC_Str value_str) {
    MC_RETURN_INVALID(composer->message == NULL);

    if(composer->message->headers_map == NULL) {
        MC_RETURN_ERROR(mc_hmap_new(composer->alloc, &composer->message->headers_map));
    }

    MC_HttpHeader *new_hdr;
    MC_RETURN_ERROR(mc_alloc(composer->alloc, sizeof *new_hdr, (void**)&new_hdr));

    MC_String *value;
    MC_Error error = mc_string(composer->alloc, &value, value_str);
    if(error) {
        mc_free(composer->alloc, new_hdr);
        return error;
    }

    MC_HMapBucket *bucket = mc_hmap_get_or_new(composer->message->headers_map, key);
    if(bucket == NULL) {
        mc_free(composer->alloc, new_hdr);
        mc_free(composer->alloc, value);
        return MCE_OUT_OF_MEMORY;
    }

    *new_hdr = (MC_HttpHeader) {
        .key = MC_STR(bucket->key, bucket->key + bucket->key_size),
        .value = mc_string_str(value),
        .next = NULL
    };

    bucket->value = new_hdr;
    composer->message->last_header = mc_list_add_after(composer->message->last_header, new_hdr);
    if(composer->message->headers == NULL) {
        composer->message->headers = composer->message->last_header;
    }

    return MCE_OK;
}

bool mc_http_is_request(const MC_HttpMessage *msg) {
    return msg->type == MESSAGE_REQUEST;
}

bool mc_http_is_response(const MC_HttpMessage *msg) {
    return msg->type == MESSAGE_RESPONSE;
}

MC_HttpVersion mc_http_get_version(const MC_HttpMessage *msg) {
    return msg->version;
}

MC_Error mc_http_get_path(const MC_HttpMessage *msg, MC_Str *path) {
    MC_RETURN_INVALID(!mc_http_is_request(msg));
    *path = mc_string_str(msg->request.path);
    return MCE_OK;
}

MC_Error mc_http_get_method(const MC_HttpMessage *msg, MC_Str *method) {
    MC_RETURN_INVALID(!mc_http_is_request(msg));
    *method = mc_strc(mc_http_method_str(msg->request.method));
    return MCE_OK;
}

MC_Error mc_http_get_code(const MC_HttpMessage *msg, unsigned *code) {
    MC_RETURN_INVALID(!mc_http_is_response(msg));
    *code = msg->response.code;
    return MCE_OK;
}

MC_Error mc_http_get_status(const MC_HttpMessage *msg, MC_Str *status) {
    MC_RETURN_INVALID(!mc_http_is_response(msg));
    if(msg->response.status) {
        *status = mc_string_str(msg->response.status);
    }
    else {
        *status = mc_strc(mc_http_code_status(msg->response.code));
    }

    return MCE_OK;
}

MC_HttpHeader *mc_http_get_headers(const MC_HttpMessage *msg) {
    return msg->headers;
}

MC_Error mc_http_get_header(const MC_HttpMessage *msg, MC_Str key, MC_Str *value) {
    MC_HttpHeader *hdr = mc_hmap_get_value(msg->headers_map, key);
    if(hdr == NULL) {
        return MCE_NOT_FOUND;
    }

    *value = hdr->value;
    return MCE_OK;
}

static MC_Error read_sized(MC_HttpReader *reader, MC_Stream *dst, size_t size, size_t *remaining) {
    *remaining = size;
    while (*remaining) {
        if(reader->bufsz) {
            size_t to_write = MIN(*remaining, reader->bufsz);
            size_t cur_wrote;
            MC_Error error = mc_write(dst, to_write, reader->buf, &cur_wrote);
            *remaining -= cur_wrote;
            MC_RETURN_ERROR(error);
        }
        else {
            size_t to_read = MIN(*remaining, sizeof reader->buf);
            MC_RETURN_ERROR(mc_read_async(reader->source, to_read, reader->buf, &reader->bufsz));
        }
    }

    return MCE_OK;
}

static MC_Error read_line(MC_HttpReader *reader, size_t max_size) {
    while (true) {
        if(reader->bufsz) {
            MC_Str buf = MC_STR(reader->buf, reader->buf + reader->bufsz);
            MC_Str delim = mc_str_substr(buf, MC_STRC("\r\n"));
            if(!mc_str_empty(delim)) {
                MC_Str line = MC_STR(buf.beg, delim.end);
                MC_RETURN_ERROR(mc_write(reader->line, mc_str_len(line) - 2, line.beg, NULL));

                size_t cursor;
                MC_RETURN_ERROR(mc_get_cursor(reader->line, &cursor));
                MC_RETURN_INVALID(cursor > max_size);

                memmove(reader->buf, line.end, reader->bufsz - mc_str_len(line));
                reader->bufsz -= mc_str_len(line);
                MC_RETURN_ERROR(mc_set_cursor(reader->line, 0, MC_CURSOR_FROM_BEG));
                return MCE_OK;
            }
            else{
                MC_RETURN_ERROR(mc_write(reader->line, reader->bufsz, reader->buf, NULL));
                reader->bufsz = 0;
            }
        }

        MC_RETURN_ERROR(mc_read_async(reader->source, sizeof reader->buf, reader->buf, &reader->bufsz));
    }
}

static MC_Error reset_line(MC_HttpReader *reader) {
    MC_RETURN_ERROR(mc_set_cursor(reader->line, 0, MC_CURSOR_FROM_BEG));
    mc_mstream_truncate(reader->line);
    return MCE_OK;
}

static MC_Error reader_begin(MC_HttpReader *reader) {
    MC_RETURN_INVALID(reader->message != NULL);
    MC_RETURN_ERROR(mc_mstream(reader->msg_alloc, &reader->line));

    reader->state = READER_FIRST_LINE;

    return MCE_OK;
}

static MC_Error read_first_line(MC_HttpReader *reader) {
    MC_RETURN_ERROR(read_line(reader, ~0));

    size_t bufsz = 0;
    char buf[128];
    MC_RETURN_ERROR(mc_read_async(reader->line, sizeof buf, buf, &bufsz));

    MC_Str chunk = MC_STR(buf, buf + bufsz);
    MC_Str delim = mc_str_substr(chunk, MC_STRC(" "));
    MC_RETURN_INVALID(mc_str_empty(delim));

    MC_Str first_word = MC_STR(chunk.beg, delim.beg);
    chunk = MC_STR(delim.end, chunk.end);
    MC_HttpMethod method = mc_http_method_from_str(first_word);
    if(method == MC_HTTP_UNDEFINED) {
        MC_HttpVersion version;
        MC_RETURN_ERROR(parse_http_version(first_word, &version));

        MC_Str delim = mc_str_substr(chunk, MC_STRC(" "));
        MC_RETURN_INVALID(mc_str_empty(delim));

        MC_Str status_code_str = MC_STR(chunk.beg, delim.beg);
        MC_RETURN_INVALID(mc_str_empty(status_code_str));
        for(const char *ch = status_code_str.beg; ch != status_code_str.end; ch++) {
            MC_RETURN_INVALID(!isdigit(*ch));
        }

        uint64_t status_code;
        mc_str_toull(status_code_str, &status_code);

        MC_Str status_str = MC_STR(delim.end, chunk.end);

        MC_HttpMessage *msg;
        MC_RETURN_ERROR(mc_alloc(reader->msg_alloc, sizeof(MC_HttpMessage), (void**)&msg));

        MC_String *status;
        MC_RETURN_ERROR(mc_string(reader->msg_alloc, &status, status_str));

        *msg = (MC_HttpMessage) {
            .type = MESSAGE_RESPONSE,
            .version = version,
            .response = {
                .code = status_code,
                .status = status
            }
        };

        reader->message = msg;
        reader->state = READER_HEADER_LINE;
        MC_RETURN_ERROR(reset_line(reader));
        return MCE_OK;
    }
    else {
        size_t line_size = mc_mstream_size(reader->line);
        MC_String *path;
        MC_RETURN_ERROR(mc_stringn(reader->msg_alloc, &path, line_size));

        for(char *cur = path->data;;) {
            MC_Str delim = mc_str_substr(chunk, MC_STRC(" "));
            MC_Str to_end = MC_STR(chunk.beg, delim.beg);
            memcpy(cur, to_end.beg, mc_str_len(to_end));
            path->len += mc_str_len(to_end);
            cur += mc_str_len(to_end);

            if(!mc_str_empty(delim)) {
                chunk.beg = delim.end;
                path->data[path->len] = '\0';
                break;
            }

            MC_RETURN_ERROR(mc_read_async(reader->line, sizeof buf, buf, &bufsz));
            chunk = MC_STR(buf, buf + bufsz);
            MC_RETURN_INVALID(mc_str_empty(chunk));
        }

        memmove(buf, chunk.beg, mc_str_len(chunk));
        bufsz = mc_str_len(chunk);
        size_t read;
        MC_RETURN_ERROR(mc_read_async(reader->line, sizeof buf - bufsz, buf + bufsz, &read));
        bufsz += read;
        MC_Str chunk = MC_STR(buf, buf + bufsz);


        MC_HttpVersion version;
        MC_RETURN_INVALID(parse_http_version(chunk, &version));

        MC_HttpMessage *msg;
        MC_RETURN_ERROR(mc_alloc(reader->msg_alloc, sizeof(MC_HttpMessage), (void**)&msg));

        *msg = (MC_HttpMessage) {
            .type = MESSAGE_REQUEST,
            .version = version,
            .request = {
                .method = method,
                .path = path
            }
        };
        reader->message = msg;
        reader->state = READER_HEADER_LINE;
        return reset_line(reader);
    }
}

static MC_Error read_header_line(MC_HttpReader *reader) {
    MC_RETURN_INVALID(reader->message == NULL);
    MC_RETURN_ERROR(read_line(reader, ~0));

    size_t line_size = mc_mstream_size(reader->line);
    if(line_size == 0) {
        reader->state = READER_BODY;
        return reset_line(reader);
    }

    size_t bufsz;
    char buf[256];
    size_t key_size = 0;
    while (true) {
        MC_RETURN_ERROR(mc_read_async(reader->line, sizeof buf, buf, &bufsz));
        MC_Str chunk = MC_STR(buf, buf + bufsz);
        MC_Str delim = mc_str_substr(chunk, MC_STRC(":"));
        if(!mc_str_empty(delim)) {
            MC_RETURN_ERROR(mc_get_cursor(reader->line, &key_size));
            key_size -= chunk.end - delim.beg;
            break;
        }
    }

    MC_RETURN_INVALID(key_size > sizeof buf);
    MC_RETURN_ERROR(mc_set_cursor(reader->line, 0, MC_CURSOR_FROM_BEG));
    MC_RETURN_ERROR(mc_read_async(reader->line, key_size, buf, &bufsz));

    MC_Str key = MC_STR(buf, buf + bufsz);
    key = mc_str_rtrim(key);
    MC_RETURN_INVALID(mc_str_empty(key));

    if(reader->message->headers_map == NULL) {
        MC_RETURN_ERROR(mc_hmap_new(reader->msg_alloc, &reader->message->headers_map));
    }

    MC_HMapBucket *bucket = mc_hmap_get_or_new(reader->message->headers_map, key);
    if(bucket == NULL) {
        return MCE_OUT_OF_MEMORY;
    }

    MC_HttpHeader *hdr;
    MC_RETURN_ERROR(mc_alloc(reader->msg_alloc, sizeof *hdr, (void**)&hdr));

    MC_String *value;
    size_t value_size = line_size - key_size - 1;
    MC_RETURN_ERROR(mc_stringn(reader->msg_alloc, &value, value_size));
    MC_RETURN_ERROR(mc_set_cursor(reader->line, key_size + 1, MC_CURSOR_FROM_BEG));
    value->len = 0;

    MC_RETURN_ERROR(mc_read_async(reader->line, sizeof buf, buf, &bufsz));
    MC_Str chunk = MC_STR(buf, buf + bufsz);
    chunk = mc_str_ltrim(chunk);
    for (char *dst = value->data; !mc_str_empty(chunk);) {
        memcpy(dst, chunk.beg, mc_str_len(chunk));
        dst += mc_str_len(chunk);
        value->len += mc_str_len(chunk);

        MC_RETURN_ERROR(mc_read_async(reader->line, sizeof buf, buf, &bufsz));
        chunk = MC_STR(buf, buf + bufsz);
    }

    value->data[value->len] = '\0';

    *hdr = (MC_HttpHeader) {
        .key = MC_STR(bucket->key, bucket->key + bucket->key_size),
        .value = mc_string_str(value)
    };
    bucket->value = hdr;

    if(reader->message->headers == NULL) {
        mc_list_add(&reader->message->headers, hdr);
        reader->message->last_header = reader->message->headers;
    }
    else {
        reader->message->last_header = mc_list_add_after(reader->message->last_header, hdr);
    }

    return reset_line(reader);
}

static MC_Error read_body(MC_HttpReader *reader, MC_Stream *body) {
    (void)body; // UNUSED

    MC_Str content_length;
    if(mc_http_get_header(reader->message, MC_STRC("Content-Length"), &content_length) == MCE_OK) {
        uint64_t body_size;
        mc_str_toull(content_length, &body_size);

        reader->body_size = body_size;
        reader->body_remaining = body_size;
        mc_fmt(MC_STDOUT, "len: %.*s; %zu\n", mc_str_len(content_length), content_length.beg, body_size);
        reader->state = READER_BODY_SIZED;
    }
    else{
        reader->state = READER_DONE;
    }

    return MCE_OK;
}

static MC_Error read_body_sized(MC_HttpReader *reader, MC_Stream *body) {
    while (reader->body_remaining) {
        MC_RETURN_ERROR(read_sized(reader, body, reader->body_remaining, &reader->body_remaining));
    }

    reader->state = READER_DONE;
    return MCE_OK;
}

static MC_Error parse_http_version(MC_Str version_str, MC_HttpVersion *version) {
    MC_Str matched, major_str, minor_str;
    MC_RETURN_INVALID(!mc_str_match(version_str, "^HTTP/(%d+)%.(%d+)", &matched, &major_str, &minor_str));
    MC_RETURN_INVALID(mc_str_len(matched) != mc_str_len(version_str));

    uint64_t major, minor;
    mc_str_toull(major_str, &major);
    mc_str_toull(minor_str, &minor);

    *version = (MC_HttpVersion){.major = major, .minor = minor};
    return MCE_OK;
}

static MC_Error reader_read(MC_HttpReader *reader, MC_Stream *body) {
    switch (reader->state) {
    case READER_BEGIN: return reader_begin(reader);
    case READER_FIRST_LINE: return read_first_line(reader);
    case READER_HEADER_LINE: return read_header_line(reader);
    case READER_BODY: return read_body(reader, body);
    case READER_BODY_SIZED: return read_body_sized(reader, body);
    default: return MCE_INVALID_INPUT;
    }
}
