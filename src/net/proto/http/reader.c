#include <mc/net/proto/http/reader.h>

#include <mc/data/arena.h>
#include <mc/data/string.h>
#include <mc/data/mstream.h>
#include <mc/util/error.h>

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

struct MC_HttpReader {
    ReaderState state;
    MC_Alloc *alloc;
    MC_Stream *source;
    MC_Alloc *tmp_alloc;
    size_t bufsz;
    char buf[256];

    MC_Arena *arena;
    MC_Stream *line;
    size_t body_size;
    size_t body_remaining;
};

static MC_Error read_sized(MC_HttpReader *reader, MC_Stream *dst, size_t size, size_t *remaining);
static MC_Error read_line(MC_HttpReader *reader, size_t max_size);
static MC_Error reset_line(MC_HttpReader *reader);
static MC_Error reader_begin(MC_HttpReader *reader);
static MC_Error read_first_line(MC_HttpReader *reader, MC_HttpMessage *msg);
static MC_Error read_header_line(MC_HttpReader *reader, MC_HttpMessage *msg);
static MC_Error read_body_sized(MC_HttpReader *reader, MC_Stream *body);
static MC_Error read_body(MC_HttpReader *reader, MC_HttpMessage *msg, MC_Stream *body);
static MC_Error parse_http_version(MC_Str src, MC_HttpVersion *version);
static MC_Error reader_read(MC_HttpReader *reader, MC_HttpMessage *msg, MC_Stream *body);

MC_Error mc_http_reader_new(MC_Alloc *alloc, MC_HttpReader **out_reader, MC_Stream *source) {
    *out_reader = NULL;

    MC_HttpReader *reader;
    MC_RETURN_ERROR(mc_alloc(alloc, sizeof(MC_HttpReader), (void**)&reader));

    MC_Arena *arena;
    MC_Error error = mc_arena_init(&arena, alloc);
    if(error != MCE_OK) {
        mc_free(alloc, reader);
        return error;
    }

    *reader = (MC_HttpReader) {
        .alloc = alloc,
        .arena = arena,
        .source = source,
        .state = READER_BEGIN,
    };
    reader->tmp_alloc = mc_arena_allocator(reader->arena);

    *out_reader = reader;
    return MCE_OK;
}

void mc_http_reader_delete(MC_HttpReader *reader) {
    if(reader == NULL) {
        return;
    }

    mc_arena_destroy(reader->arena);
    mc_free(reader->alloc, reader);
}

MC_Error mc_http_reader_read(MC_HttpReader *reader, MC_HttpMessage *message, MC_Stream *body) {
    while (reader->state != READER_DONE) {
        MC_Error status = reader_read(reader, message, body);
        if(status != MCE_OK) {
            if(status != MCE_AGAIN) {
                reader->state = READER_ERROR;
            }

            return status;
        }
    }

    return MCE_OK;
}

MC_Error mc_http_reader_finish(MC_HttpReader *reader) {
    while(reader->state & READERF_INCOMPLETE ) {
        MC_RETURN_ERROR(mc_http_reader_read(reader, NULL, NULL));
    }

    mc_arena_reset(reader->arena);
    MC_RETURN_INVALID(reader->state == READER_ERROR);

    reader->state = READER_BEGIN;
    reader->line = NULL;
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
    MC_RETURN_ERROR(mc_mstream(reader->tmp_alloc, &reader->line));
    reader->state = READER_FIRST_LINE;
    return MCE_OK;
}

static MC_Error read_first_line(MC_HttpReader *reader, MC_HttpMessage *msg) {
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
        mc_http_set_version(msg, version);

        MC_Str delim = mc_str_substr(chunk, MC_STRC(" "));
        MC_RETURN_INVALID(mc_str_empty(delim));

        MC_Str status_code_str = MC_STR(chunk.beg, delim.beg);
        MC_RETURN_INVALID(mc_str_empty(status_code_str));
        for(const char *ch = status_code_str.beg; ch != status_code_str.end; ch++) {
            MC_RETURN_INVALID(!isdigit(*ch));
        }

        uint64_t status_code;
        mc_str_toull(status_code_str, &status_code);

        // MC_Str status_str = MC_STR(delim.end, chunk.end);

        mc_http_set_response(msg, status_code);

        reader->state = READER_HEADER_LINE;
        MC_RETURN_ERROR(reset_line(reader));
        return MCE_OK;
    }
    else {
        size_t line_size = mc_mstream_size(reader->line);
        MC_String *path;
        MC_RETURN_ERROR(mc_stringn(reader->tmp_alloc, &path, line_size));

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
        mc_http_set_version(msg, version);
        MC_RETURN_ERROR(mc_http_set_request(msg, method, mc_string_str(path)));

        reader->state = READER_HEADER_LINE;
        return reset_line(reader);
    }
}

static MC_Error read_header_line(MC_HttpReader *reader, MC_HttpMessage *msg) {
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

    MC_String *key;
    MC_RETURN_ERROR(mc_string(reader->tmp_alloc, &key, mc_str_rtrim(MC_STR(buf, buf + bufsz))));

    MC_String *value;
    size_t value_size = line_size - key_size - 1;
    MC_RETURN_ERROR(mc_stringn(reader->tmp_alloc, &value, value_size));
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

    MC_RETURN_ERROR(mc_http_set_header(msg, mc_string_str(key), mc_string_str(value)));

    return reset_line(reader);
}

static MC_Error read_body(MC_HttpReader *reader, MC_HttpMessage *msg, MC_Stream *body) {
    (void)body; // UNUSED

    MC_Str content_length;
    if(mc_http_get_header(msg, MC_STRC("Content-Length"), &content_length) == MCE_OK) {
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

static MC_Error reader_read(MC_HttpReader *reader, MC_HttpMessage *msg, MC_Stream *body) {
    switch (reader->state) {
    case READER_BEGIN: return reader_begin(reader);
    case READER_FIRST_LINE: return read_first_line(reader, msg);
    case READER_HEADER_LINE: return read_header_line(reader, msg);
    case READER_BODY: return read_body(reader, msg, body);
    case READER_BODY_SIZED: return read_body_sized(reader, body);
    default: return MCE_INVALID_INPUT;
    }
}
