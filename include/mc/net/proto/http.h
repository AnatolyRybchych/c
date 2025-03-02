#ifndef MC_NET_PROTO_HTTP_H
#define MC_NET_PROTO_HTTP_H

#include <stdint.h>
#include <mc/data/string.h>
#include <mc/data/hmap.h>
#include <mc/data/stream.h>
#include <mc/error.h>
#include <mc/util/enum.h>

#define MC_ITER_HTTP_METHODS(CB, ...) \
    CB(GET, ##__VA_ARGS__) \
    CB(HEAD, ##__VA_ARGS__) \
    CB(POST, ##__VA_ARGS__) \
    CB(PUT, ##__VA_ARGS__) \
    CB(DELETE, ##__VA_ARGS__) \
    CB(CONNECT, ##__VA_ARGS__) \
    CB(OPTIONS, ##__VA_ARGS__) \
    CB(TRACE, ##__VA_ARGS__) \
    CB(PATCH, ##__VA_ARGS__) \

typedef struct MC_HttpReader MC_HttpReader;
typedef struct MC_HttpWriter MC_HttpWriter;
typedef struct MC_HttpComposer MC_HttpComposer;
typedef struct MC_HttpVersion MC_HttpVersion;
typedef struct MC_HttpMessage MC_HttpMessage;
typedef struct MC_HttpKV MC_HttpKV, MC_HttpHeader, MC_HttpCookie;

typedef unsigned MC_HttpMethod;
enum MC_HttpMethod{
    MC_HTTP_UNDEFINED,
    MC_ITER_HTTP_METHODS(MC_DEFINE_ENUM_ITEM, MC_HTTP_)
};

MC_Error mc_http_reader_new(MC_Alloc *alloc, MC_HttpReader **reader, MC_Stream *source);
void mc_http_reader_delete(MC_HttpReader *reader);
MC_Error mc_http_reader_read(MC_HttpReader *reader, const MC_HttpMessage **message, MC_Stream *body);
MC_Error mc_http_reader_finish(MC_HttpReader *reader);

MC_Error mc_http_writer_new(MC_Alloc *alloc, MC_HttpWriter **writer);
void mc_http_writer_delete(MC_HttpWriter *writer);
MC_Error mc_http_writer_beg(MC_HttpWriter *writer, MC_Stream *destination, const MC_HttpMessage *message);
MC_Error mc_http_writer_write(MC_HttpWriter *writer);

const char *mc_http_code_status(unsigned code);
const char *mc_http_method_str(MC_HttpMethod method);
MC_HttpMethod mc_http_method_from_str(MC_Str str);

MC_Error mc_http_composer_new(MC_Alloc *alloc, MC_HttpComposer **composer);
void mc_http_composer_delete(MC_HttpComposer *composer);
MC_Error mc_http_composer_get_message(MC_HttpComposer *composer, const MC_HttpMessage **message);
MC_Error mc_http_composer_begin_request(MC_HttpComposer *composer);
MC_Error mc_http_composer_begin_response(MC_HttpComposer *composer);
MC_Error mc_http_composer_set_status_code(MC_HttpComposer *composer, int code);
MC_Error mc_http_composer_set_method(MC_HttpComposer *composer, MC_HttpMethod method);
MC_Error mc_http_composer_set_path(MC_HttpComposer *composer, MC_Str path);
MC_Error mc_http_composer_set_version(MC_HttpComposer *composer, MC_HttpVersion version);
MC_Error mc_http_composer_set_headers(MC_HttpComposer *composer, const MC_HttpHeader *headers);
MC_Error mc_http_composer_set_header(MC_HttpComposer *composer, MC_Str key, MC_Str value);

bool mc_http_is_request(const MC_HttpMessage *msg);
bool mc_http_is_response(const MC_HttpMessage *msg);
MC_HttpVersion mc_http_get_version(const MC_HttpMessage *msg);
MC_Error mc_http_get_path(const MC_HttpMessage *msg, MC_Str *path);
MC_Error mc_http_get_method(const MC_HttpMessage *msg, MC_Str *method);
MC_Error mc_http_get_code(const MC_HttpMessage *msg, unsigned *code);
MC_Error mc_http_get_status(const MC_HttpMessage *msg, MC_Str *status);
MC_HttpHeader *mc_http_get_headers(const MC_HttpMessage *msg);
MC_Error mc_http_get_header(const MC_HttpMessage *msg, MC_Str key, MC_Str *value);

struct MC_HttpKV {
    MC_HttpHeader *next;
    MC_Str key;
    MC_Str value;
};

struct MC_HttpVersion {
    uint16_t major;
    uint16_t minor;
};

#endif // MC_NET_PROTO_HTTP_H
