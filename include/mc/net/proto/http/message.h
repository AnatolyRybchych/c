#ifndef MC_NET_PROTO_HTTP_MESSAGE_H
#define MC_NET_PROTO_HTTP_MESSAGE_H

#include <mc/error.h>
#include <mc/data/str.h>
#include <mc/data/alloc.h>
#include <mc/data/hmap.h>
#include <mc/util/enum.h>

#include <stdbool.h>

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

typedef unsigned MC_HttpMethod;
enum MC_HttpMethod{
    MC_HTTP_UNDEFINED,
    MC_ITER_HTTP_METHODS(MC_DEFINE_ENUM_ITEM, MC_HTTP_)
};

typedef struct MC_HttpMessage MC_HttpMessage;
typedef struct MC_HttpVersion MC_HttpVersion;

MC_Error mc_http_message_create(MC_Alloc *alloc, MC_HttpMessage **msg);
void mc_http_message_delete(MC_HttpMessage *msg);

MC_Error mc_http_set_request(MC_HttpMessage *msg, MC_HttpMethod method, MC_Str path);
MC_Error mc_http_get_path(const MC_HttpMessage *msg, MC_Str *path);
MC_Error mc_http_get_method(const MC_HttpMessage *msg, MC_HttpMethod *method);
bool mc_http_is_request(const MC_HttpMessage *msg);

void mc_http_set_response(MC_HttpMessage *msg, unsigned code);
MC_Error mc_http_get_code(const MC_HttpMessage *msg, unsigned *code);
bool mc_http_is_response(const MC_HttpMessage *msg);

void mc_http_set_version(MC_HttpMessage *msg, MC_HttpVersion version);
MC_HttpVersion mc_http_get_version(const MC_HttpMessage *msg);

MC_HMap *mc_http_headers(MC_HttpMessage *msg);
MC_Error mc_http_get_header(const MC_HttpMessage *msg, MC_Str key, MC_Str *value);
MC_Error mc_http_set_header(MC_HttpMessage *msg, MC_Str key, MC_Str value);
MC_Error mc_http_delete_header(MC_HttpMessage *msg, MC_Str key);

const char *mc_http_code_status(unsigned code);
MC_HttpMethod mc_http_method_from_str(MC_Str str);
const char *mc_http_method_str(MC_HttpMethod method);

struct MC_HttpVersion {
    uint16_t major;
    uint16_t minor;
};

#endif // MC_NET_PROTO_HTTP_MESSAGE_H
