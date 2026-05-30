#include <mc/net/proto/http/message.h>

#include <mc/data/hmap.h>
#include <mc/data/string.h>

#include <mc/util/error.h>

typedef unsigned MC_HttpMessageType;
enum MC_HttpMessageType {
    MESSAGE_UNKNOWN,
    MESSAGE_REQUEST,
    MESSAGE_RESPONSE,
};

struct MC_HttpMessage {
    MC_Alloc *alloc;

    MC_HttpVersion version;
    MC_HttpMessageType type;

    union {
        struct {
            MC_HttpMethod method;
            MC_String *path;
        } request;

        struct {
            unsigned code;
        } response;
    };

    MC_HMap *headers;
};

MC_Error mc_http_message_create(MC_Alloc *alloc, MC_HttpMessage **msg) {
    *msg = NULL;

    MC_HMap *headers;
    MC_RETURN_ERROR(mc_hmap_new(alloc, &headers));

    MC_HttpMessage *new;
    MC_Error error = mc_alloc(alloc, sizeof(MC_HttpMessage), (void**)&new);
    if(error) {
        mc_hmap_delete(headers);
        return error;
    }

    *new = (MC_HttpMessage){
        .alloc = alloc,
        .headers = headers,
        .version = {.major = 1, .minor = 1}
    };

    *msg = new;
    return MCE_OK;
}

void mc_http_message_delete(MC_HttpMessage *msg) {
    mc_hmap_delete(msg->headers);
    mc_free(msg->alloc, msg);
}

MC_Error mc_http_set_request(MC_HttpMessage *msg, MC_HttpMethod method, MC_Str path) {
    MC_String *request_path;
    MC_RETURN_ERROR(mc_string(msg->alloc, &request_path, path));

    if(msg->type == MESSAGE_REQUEST) {
        mc_free(msg->alloc, msg->request.path);
    }

    msg->type = MESSAGE_REQUEST;
    msg->request.method = method;
    msg->request.path = request_path;
    return MCE_OK;
}

bool mc_http_is_request(const MC_HttpMessage *msg) {
    return msg->type == MESSAGE_REQUEST;
}

void mc_http_set_response(MC_HttpMessage *msg, unsigned code) {
    msg->type = MESSAGE_RESPONSE;
    msg->response.code = code;

    if(msg->type == MESSAGE_REQUEST) {
        mc_free(msg->alloc, msg->request.path);
    }
}

bool mc_http_is_response(const MC_HttpMessage *msg) {
    return msg->type == MESSAGE_RESPONSE;
}

MC_HttpVersion mc_http_get_version(const MC_HttpMessage *msg) {
    return msg->version;
}

void mc_http_set_version(MC_HttpMessage *msg, MC_HttpVersion version) {
    msg->version = version;
}

MC_Error mc_http_get_path(const MC_HttpMessage *msg, MC_Str *path) {
    MC_RETURN_INVALID(msg->type != MESSAGE_REQUEST);

    *path = mc_string_str(msg->request.path);
    return MCE_OK;
}

MC_Error mc_http_get_method(const MC_HttpMessage *msg, MC_HttpMethod *method) {
    MC_RETURN_INVALID(msg->type != MESSAGE_REQUEST);

    *method = msg->request.method;
    return MCE_OK;
}
MC_Error mc_http_get_code(const MC_HttpMessage *msg, unsigned *code) {
    MC_RETURN_INVALID(msg->type != MESSAGE_RESPONSE);

    *code = msg->response.code;
    return MCE_OK;
}

const MC_HMap *mc_http_get_headers(const MC_HttpMessage *msg) {
    return msg->headers;
}

MC_Error mc_http_get_header(const MC_HttpMessage *msg, MC_Str key, MC_Str *value) {
    MC_HMapBucket *found = mc_hmap_get(msg->headers, key);
    if(!found) {
        return MCE_NOT_FOUND;
    }

    *value = mc_string_str(found->value);
    return MCE_OK;
}

MC_HMap *mc_http_headers(MC_HttpMessage *msg) {
    return msg->headers;
}

MC_Error mc_http_set_header(MC_HttpMessage *msg, MC_Str key, MC_Str value) {
    MC_HMapBucket *header = mc_hmap_get_or_new(msg->headers, key);
    if(!header) {
        return MCE_OUT_OF_MEMORY;
    }

    MC_RETURN_ERROR(mc_string(msg->alloc, (MC_String**)&header->value, value));
    return MCE_OK;
}

MC_Error mc_http_delete_header(MC_HttpMessage *msg, MC_Str key) {
    return mc_hmap_del(msg->headers, key);
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

MC_HttpMethod mc_http_method_from_str(MC_Str str) {
    MC_HttpMethod result;
    MC_ENUM_FROM_STR(result, str, MC_ITER_HTTP_METHODS, MC_HTTP_){
        return MC_HTTP_UNDEFINED;
    };
    return result;
}

const char *mc_http_method_str(MC_HttpMethod method) {
    const char *result = NULL;
    MC_ENUM_TO_STRING(result, method, MC_ITER_HTTP_METHODS, MC_HTTP_)
    return result;
}
