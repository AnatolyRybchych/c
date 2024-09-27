#include <mc/data/json.h>

#include <assert.h>
#include <memory.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

#define INITIAL_LIST_SIZE 8

#define OPTIONAL_SET(DST, ...) if(DST) *(DST) = __VA_ARGS__

typedef unsigned SubType;
enum SubType{
    SUBTYPE_NONE,
    SUBTYPE_U64,
    SUBTYPE_I64,
    SUBTYPE_LF,
    SUBTYPE_ARRAY,
    SUBTYPE_KEY_VALUE_ARRAY,
};

struct String{
    size_t length;
    char data[];
};

struct Array{
    size_t capacity;
    size_t size;
    MC_Json *items[];
};

struct Kv{
    struct String *key;
    MC_Json *value;
};

struct KvArray{
    size_t capacity;
    size_t size;
    struct Kv kvs[];
};

struct MC_Json{
    MC_Alloc *alloc;
    uint8_t type;
    uint8_t subtype;
    union{
        uint64_t u64;
        int64_t i64;
        double lf;
        struct String *string;
        struct Array *arr;
        struct KvArray *kv_arr;
    } as;
};

struct Parser{
    MC_Alloc *alloc;
    MC_Str src;
    MC_Str cur;
};

static void reset(MC_Json *json);
static void delete_arr(MC_Alloc *alloc, struct Array *arr);
static void delete_kv_arr(MC_Alloc *alloc, struct KvArray *kv_arr);

static MC_Error json_dump(MC_Json *json, MC_Stream *out, MC_Str ident, unsigned ident_cnt);
static MC_Error loads(MC_Json **json, struct Parser *p);
static MC_Error loads_object(MC_Json **ret_json, struct Parser *p);
static MC_Error loads_list(MC_Json **ret_json, struct Parser *p);
static MC_Error loads_number(MC_Json **ret_json, struct Parser *p);
static MC_Error loads_bool(MC_Json **ret_json, struct Parser *p);
static MC_Error loads_null(MC_Json **ret_json, struct Parser *p);
static MC_Error loads_string(MC_Json **ret_json, struct Parser *p);
static MC_Error parse_string(struct String **ret_string, struct Parser *p);
static MC_Error dump_ident(MC_Stream *out, MC_Str ident, unsigned ident_cnt);
static MC_Error dump_number(MC_Json *number, MC_Stream *out);
static MC_Error dump_array(struct Array *arr, MC_Stream *out, MC_Str ident, unsigned ident_cnt);
static MC_Error dump_kv_array(struct KvArray *arr, MC_Stream *out, MC_Str ident, unsigned ident_cnt);
static MC_Error dump_string(const struct String *string, MC_Stream *out);
static MC_Error string_fmt(MC_Alloc *alloc, struct String **string, const char *fmt, va_list args);
static MC_Error object_add(MC_Json *json, struct String *key, MC_Json *item);

MC_Error mc_json_new(MC_Alloc *alloc, MC_Json **ret_json){
    MC_Json *json;
    MC_RETURN_ERROR(mc_alloc(alloc, sizeof(MC_Json), (void*)&json));

    *ret_json = json;
    *json = (MC_Json){
        .alloc = alloc,
        .type = MC_JSON_NULL
    };
    return MCE_OK;
}

void mc_json_delete(MC_Json **json_ptr){
    if(json_ptr == NULL || *json_ptr == NULL){
        return;
    }

    MC_Json *json = *json_ptr;
    *json_ptr = NULL;

    reset(json);
    mc_free(json->alloc, json);
}

MC_JsonType mc_json_type(MC_Json *json){
    return json == NULL ? MC_JSON_NULL : json->type;
}

size_t mc_json_length(MC_Json *json){
    switch (mc_json_type(json)){
    case MC_JSON_LIST:
        assert(json->subtype == SUBTYPE_ARRAY);
        return json->as.arr ? json->as.arr->size : 0;
    case MC_JSON_STRING:
        return json->as.string ? json->as.string->length : 0;
    case MC_JSON_OBJECT:
        assert(json->subtype == SUBTYPE_KEY_VALUE_ARRAY);
        return json->as.kv_arr ? json->as.kv_arr->size : 0;
    default:
        return 0;
    }
}

MC_Error mc_json_set_bool(MC_Json *json, bool value){
    MC_RETURN_INVALID(json == NULL);

    reset(json);
    json->type = MC_JSON_BOOL;
    json->as.u64 = value ? 1 : 0;
    return MCE_OK;
}

MC_Error mc_json_set_u64(MC_Json *json, uint64_t value){
    MC_RETURN_INVALID(json == NULL);

    reset(json);
    json->type = MC_JSON_NUMBER;
    json->subtype = SUBTYPE_U64;
    json->as.u64 = value;
    return MCE_OK;
}

MC_Error mc_json_set_i64(MC_Json *json, int64_t value){
    MC_RETURN_INVALID(json == NULL);

    reset(json);
    json->type = MC_JSON_NUMBER;
    json->subtype = SUBTYPE_I64;
    json->as.i64 = value;
    return MCE_OK;
}

MC_Error mc_json_set_lf(MC_Json *json, double value){
    MC_RETURN_INVALID(json == NULL);

    reset(json);
    json->type = MC_JSON_NUMBER;
    json->subtype = SUBTYPE_LF;
    json->as.lf = value;
    return MCE_OK;
}

MC_Error mc_json_set_string(MC_Json *json, MC_Str str){
    MC_RETURN_INVALID(json == NULL);

    struct String *string;
    MC_RETURN_ERROR(mc_alloc(json->alloc, mc_str_len(str) + 1, (void**)&string));
    string->length = mc_str_len(str);
    memcpy(string->data, str.beg, mc_str_len(str));
    string->data[string->length] = '\0';

    reset(json);
    json->type = MC_JSON_STRING;
    json->as.string = string;
    return MCE_OK;
}

MC_Error mc_json_set_stringf(MC_Json *json, const char *fmt, ...){
    MC_RETURN_INVALID(json == NULL);

    va_list args;
    va_start(args, fmt);
    struct String *string = NULL;
    MC_Error status = string_fmt(json->alloc, &string, fmt, args);
    va_end(args);

    MC_RETURN_ERROR(status);

    reset(json);
    json->type = MC_JSON_STRING;
    json->as.string = string;
    return MCE_OK;
}

MC_Error mc_json_set_null(MC_Json *json){
    MC_RETURN_INVALID(json == NULL);

    reset(json);
    return MCE_OK;
}

MC_Error mc_json_set_list(MC_Json *json){
    MC_RETURN_INVALID(json == NULL);

    json->type = MC_JSON_LIST;
    json->subtype = SUBTYPE_ARRAY;
    json->as.arr = NULL;
    return MCE_OK;
}

MC_Error mc_json_set_object(MC_Json *json){
    MC_RETURN_INVALID(json == NULL);

    json->type = MC_JSON_OBJECT;
    json->subtype = SUBTYPE_KEY_VALUE_ARRAY;
    json->as.kv_arr = NULL;
    return MCE_OK;
}

MC_Error mc_json_list_add_new(MC_Json *json, MC_Json **item){
    MC_RETURN_INVALID(json == NULL);
    MC_RETURN_ERROR(mc_json_new(json->alloc, item));
    MC_Error status = mc_json_list_add(json, *item);
    if(status != MCE_OK){
        mc_json_delete(item);
    }

    return status;
}

MC_Error mc_json_list_add(MC_Json *json, MC_Json *item){
    MC_RETURN_INVALID(json == NULL);

    if(json->type != MC_JSON_LIST){
        MC_RETURN_INVALID(json->type != MC_JSON_NULL);
        MC_RETURN_ERROR(mc_json_set_list(json));
    }

    assert(json->subtype == SUBTYPE_ARRAY);

    if(json->as.arr == NULL){
        MC_RETURN_ERROR(mc_alloc(json->alloc, sizeof(struct Array) + sizeof(MC_Json*[INITIAL_LIST_SIZE]), (void**)&json->as.arr));
        json->as.arr->size = 0;
        json->as.arr->capacity = INITIAL_LIST_SIZE;
    }
    else if(json->as.arr->size == json->as.arr->capacity){
        struct Array *new_array;
        size_t new_capacity = json->as.arr->size * 2;
        MC_RETURN_ERROR(mc_alloc(json->alloc, sizeof(struct Array) + sizeof(MC_Json*[new_capacity]), (void**)&new_array));

        memcpy(new_array, json->as.arr, sizeof(struct Array) + sizeof(MC_Json*[json->as.arr->size]));
        new_array->capacity = new_capacity;
        mc_free(json->alloc, json->as.arr);
        json->as.arr = new_array;
    }

    if(item == NULL){
        MC_RETURN_ERROR(mc_json_new(json->alloc, &item));
    }

    json->as.arr->items[json->as.arr->size++] = item;
    return MCE_OK;
}

MC_Error mc_json_list_load(MC_Json *json, MC_Str str){
    MC_RETURN_INVALID(json == NULL);

    MC_Json *item;
    MC_RETURN_ERROR(mc_json_loads(json->alloc, &item, str));

    MC_Error status = mc_json_list_add(json, item);
    if(status != MCE_OK){
        mc_json_delete(&item);
    }

    return status;
}

MC_Error mc_json_object_add(MC_Json *json, MC_Json *item, const char *key_fmt, ...){
    va_list args;
    va_start(args, key_fmt);
    MC_Error status = mc_json_object_addv(json, item, key_fmt, args);
    va_end(args);
    return status;
}

MC_Error mc_json_object_addv(MC_Json *json, MC_Json *item, const char *key_fmt, va_list args){
    MC_RETURN_INVALID(json == NULL);

    struct String *key;
    MC_RETURN_ERROR(string_fmt(json->alloc, &key, key_fmt, args));

    MC_Error status = object_add(json, key, item);
    if(status != MCE_OK){
        mc_free(json->alloc, key);
    }

    return status;
}

MC_Error mc_json_object_add_new(MC_Json *json, MC_Json **item, const char *key_fmt, ...){
    MC_RETURN_INVALID(json == NULL);
    MC_RETURN_ERROR(mc_json_new(json->alloc, item));

    va_list args;
    va_start(args, key_fmt);
    MC_Error status = mc_json_object_addv(json, *item, key_fmt, args);
    va_end(args);

    if(status != MCE_OK){
        mc_json_delete(item);
    }

    return status;
}

MC_Error mc_json_dump(MC_Json *json, MC_Stream *out){
    return json_dump(json, out, MC_STRC("  "), 0);
}

MC_Error mc_json_loads(MC_Alloc *alloc, MC_Json **ret_json, MC_Str str){
    struct Parser p = {
        .alloc = alloc,
        .src = str,
        .cur = mc_str_trim(str),
    };

    return loads(ret_json, &p);
}

static void reset(MC_Json *json){
    switch (json->type){
    case MC_JSON_STRING:
        mc_free(json->alloc, json->as.string);
        break;
    case MC_JSON_LIST:
        assert(json->subtype == SUBTYPE_ARRAY);
        delete_arr(json->alloc, json->as.arr);
        break;
    case MC_JSON_OBJECT:
        assert(json->subtype == SUBTYPE_KEY_VALUE_ARRAY);
        delete_kv_arr(json->alloc, json->as.kv_arr);
        break;
    case MC_JSON_NULL:
    case MC_JSON_NUMBER:
    case MC_JSON_BOOL:
        break;
    default:
        break;
    }

    json->type = MC_JSON_NULL;
}

static void delete_arr(MC_Alloc *alloc, struct Array *arr){
    if(arr == NULL){
        return;
    }

    for(MC_Json **el = arr->items, **end = arr->items + arr->size; el != end; el++){
        mc_json_delete(el);
    }
    
    mc_free(alloc, arr);
}

static void delete_kv_arr(MC_Alloc *alloc, struct KvArray *kv_arr){
    if(kv_arr == NULL){
        return;
    }

    for(struct Kv *kv = kv_arr->kvs, *end = kv_arr->kvs + kv_arr->size; kv != end; kv++){
        mc_free(alloc, kv->key);
        mc_json_delete(&kv->value);
    }

    mc_free(alloc, kv_arr);
}

static MC_Error json_dump(MC_Json *json, MC_Stream *out, MC_Str ident, unsigned ident_cnt){
    switch (mc_json_type(json)){
    case MC_JSON_NULL:
        return mc_fmt(out, "null");
    case MC_JSON_BOOL:
        return mc_fmt(out, "%s", json->as.i64 ? "true": "false");
    case MC_JSON_STRING:
        return dump_string(json->as.string, out);
    case MC_JSON_LIST:
        assert(json->subtype == SUBTYPE_ARRAY);
        return dump_array(json->as.arr, out, ident, ident_cnt);
    case MC_JSON_OBJECT:
        assert(json->subtype == SUBTYPE_KEY_VALUE_ARRAY);
        return dump_kv_array(json->as.kv_arr, out, ident, ident_cnt);
    case MC_JSON_NUMBER:
        return dump_number(json, out);
    default:
        return MCE_INVALID_INPUT;
    }
}

static MC_Error loads(MC_Json **ret_json, struct Parser *p){
    if(mc_str_empty(p->cur)){
        return MCE_INVALID_FORMAT;
    }

    switch (*p->cur.beg){
    case '{':
        return loads_object(ret_json, p);
    case '[':
        return loads_list(ret_json, p);
    case '\"':
        return loads_string(ret_json, p);
    case '0': case '1': case '2': case '3': case '4': 
    case '5': case '6': case '7': case '8': case '9':
        return loads_number(ret_json, p);
    case 't':case 'f':
        return loads_bool(ret_json, p);
    case 'n':
        return loads_null(ret_json, p);
    default:
        return MCE_INVALID_FORMAT;
    }
}

static MC_Error loads_object(MC_Json **ret_json, struct Parser *p){
    *ret_json = NULL;

    if(mc_str_empty(p->cur) || *p->cur.beg != '{'){
        return MCE_INVALID_FORMAT;
    }

    p->cur.beg++;

    MC_Json *obj;
    MC_RETURN_ERROR(mc_json_new(p->alloc, &obj));
    MC_Error status = mc_json_set_object(obj);
    if(status != MCE_OK){
        mc_json_delete(&obj);
        return status;
    }

    p->cur = mc_str_ltrim(p->cur);
    if(mc_str_len(p->cur) && *p->cur.beg == '}'){
        p->cur.beg++;
        *ret_json = obj;
        return MCE_OK;
    }

    while(true){
        struct String *key;
        status = parse_string(&key, p);
        if(status != MCE_OK){
            mc_json_delete(&obj);
            return status;
        }

        p->cur = mc_str_ltrim(p->cur);

        if(mc_str_empty(p->cur) || *p->cur.beg != ':'){
            mc_json_delete(&obj);
            mc_free(p->alloc, key);
            return MCE_INVALID_FORMAT;
        }

        p->cur.beg++;
        p->cur = mc_str_ltrim(p->cur);

        MC_Json *item;
        status = loads(&item, p);
        if(status != MCE_OK){
            mc_json_delete(&obj);
            mc_free(p->alloc, key);
            return status;
        }

        status = object_add(obj, key, item);
        if(status != MCE_OK){
            mc_json_delete(&obj);
            mc_free(p->alloc, key);
            return status;
        }

        p->cur = mc_str_ltrim(p->cur);
        if(mc_str_empty(p->cur)){
            return MCE_INVALID_FORMAT;
        }

        if(*p->cur.beg == '}'){
            p->cur.beg++;
            *ret_json = obj;
            return MCE_OK;
        }
        else if(*p->cur.beg != ','){
            return MCE_INVALID_FORMAT;
        }

        p->cur.beg++;
        p->cur = mc_str_ltrim(p->cur);
    }
}

static MC_Error loads_list(MC_Json **ret_json, struct Parser *p){
    *ret_json = NULL;

    if(mc_str_empty(p->cur) || *p->cur.beg != '['){
        return MCE_INVALID_FORMAT;
    }

    p->cur.beg++;

    MC_Json *list;
    MC_RETURN_ERROR(mc_json_new(p->alloc, &list));
    MC_Error status = mc_json_set_list(list);
    if(status != MCE_OK){
        mc_json_delete(&list);
        return status;
    }

    p->cur = mc_str_ltrim(p->cur);
    if(mc_str_len(p->cur) && *p->cur.beg == ']'){
        p->cur.beg++;
        *ret_json = list;
        return MCE_OK;
    }

    while(true){
        MC_Json *item;
        status = loads(&item, p);
        if(status != MCE_OK){
            mc_json_delete(&list);
            return status;
        }

        status = mc_json_list_add(list, item);
        if(status != MCE_OK){
            mc_json_delete(&list);
            return status;
        }

        p->cur = mc_str_ltrim(p->cur);
        if(mc_str_empty(p->cur)){
            return MCE_INVALID_FORMAT;
        }

        if(*p->cur.beg == ']'){
            p->cur.beg++;
            *ret_json = list;
            return MCE_OK;
        }
        else if(*p->cur.beg != ','){
            return MCE_INVALID_FORMAT;
        }

        p->cur.beg++;
        p->cur = mc_str_ltrim(p->cur);
    }
}

static MC_Error loads_number(MC_Json **ret_json, struct Parser *p){
    *ret_json = NULL;

    uint64_t num;
    p->cur = mc_str_toull(p->cur, &num);

    if(mc_str_empty(p->cur) || *p->cur.beg != '.'){
        MC_RETURN_ERROR(mc_json_new(p->alloc, ret_json));
        return mc_json_set_u64(*ret_json, num);
    }

    p->cur.beg++;

    uint64_t num2;
    MC_Str rest = mc_str_toull(p->cur, &num2);
    size_t len = rest.beg - p->cur.beg;
    p->cur = rest;

    double value = (double)num + num2 * pow(0.1, len);
    MC_RETURN_ERROR(mc_json_new(p->alloc, ret_json));
    return mc_json_set_lf(*ret_json, value);
}

static MC_Error loads_bool(MC_Json **ret_json, struct Parser *p){
    *ret_json = NULL;

    if(mc_str_starts_with(p->cur, MC_STRC("true"))){
        p->cur.beg += mc_str_len(MC_STRC("true"));
        MC_RETURN_ERROR(mc_json_new(p->alloc, ret_json));
        return mc_json_set_bool(*ret_json, true);
    }
    else if(mc_str_starts_with(p->cur, MC_STRC("false"))){
        p->cur.beg += mc_str_len(MC_STRC("false"));
        MC_RETURN_ERROR(mc_json_new(p->alloc, ret_json));
        return mc_json_set_bool(*ret_json, false);
    }

    return MCE_INVALID_FORMAT;
}

static MC_Error loads_null(MC_Json **ret_json, struct Parser *p){
    *ret_json = NULL;

    if(mc_str_starts_with(p->cur, MC_STRC("null"))){
        p->cur.beg += mc_str_len(MC_STRC("null"));
        return mc_json_new(p->alloc, ret_json);
    }

    return MCE_INVALID_FORMAT;
}

static MC_Error loads_string(MC_Json **ret_json, struct Parser *p){
    struct String *ret_string;
    MC_RETURN_ERROR(parse_string(&ret_string, p));
    
    MC_Error status = mc_json_new(p->alloc, ret_json);
    if(status != MCE_OK){
        mc_free(p->alloc, ret_string);
        return status;
    }

    MC_Json *json = *ret_json;
    json->type = MC_JSON_STRING;
    json->as.string = ret_string;
    return MCE_OK;
}

static MC_Error parse_string(struct String **ret_string, struct Parser *p){
    if(mc_str_len(p->cur) < 2 || *p->cur.beg != '\"'){
        return MCE_INVALID_FORMAT;
    }

    const char *string_end = p->cur.beg + 1;
    for(;*string_end != '\"'; string_end++){
        if(string_end == p->cur.end){
            return MCE_INVALID_FORMAT;
        }

        if(*string_end == '\\'){
            ++string_end;
            if(string_end == p->cur.end){
                return MCE_INVALID_FORMAT;
            }
        }
    }

    struct String *string;
    MC_RETURN_ERROR(mc_alloc(p->alloc, sizeof(struct String) + string_end - p->cur.beg, (void**)&string));
    
    char *dst = string->data;
    for(const char *ch = p->cur.beg + 1; ch != string_end; ch++){
        if(*ch != '\\'){
            *dst++ = *ch;
            continue;
        }

        switch (*ch++){
        case '\\': *dst++ = '\\'; break;
        case '\"': *dst++ = '\"'; break;
        case 't': *dst++ = '\t'; break;
        case 'n': *dst++ = '\n'; break;
        case 'b': *dst++ = '\b'; break;
        case 'f': *dst++ = '\f'; break;
        case '0': *dst++ = '\0'; break;
        default: return MCE_INVALID_FORMAT;
        }
    }

    p->cur.beg = string_end + 1;

    string->length = dst - string->data;
    *ret_string = string;
    return MCE_OK;
}

static MC_Error dump_ident(MC_Stream *out, MC_Str ident, unsigned ident_cnt){
    while (ident_cnt--){
        MC_RETURN_ERROR(mc_fmt(out, "%.*s", (int)mc_str_len(ident), ident.beg));
    }

    return MCE_OK;
}

static MC_Error dump_number(MC_Json *number, MC_Stream *out){
    switch (number->subtype){
    case SUBTYPE_U64: return mc_fmt(out, "%llu", number->as.u64);
    case SUBTYPE_I64: return mc_fmt(out, "%lli", number->as.i64);
    case SUBTYPE_LF: return mc_fmt(out, "%lf", number->as.lf);
    default: return MCE_INVALID_INPUT;
    }
}

static MC_Error dump_array(struct Array *arr, MC_Stream *out, MC_Str ident, unsigned ident_cnt){
    if(arr == NULL || arr->size == 0){
        return mc_fmt(out, "[]");
    }

    MC_RETURN_ERROR(mc_fmt(out, "[\n"));
    MC_RETURN_ERROR(dump_ident(out, ident, ident_cnt + 1));
    for(MC_Json **it = arr->items, **end = arr->items + arr->size; it != end; it++){
        if(it != arr->items){
            MC_RETURN_ERROR(mc_fmt(out, ",\n"));
            MC_RETURN_ERROR(dump_ident(out, ident, ident_cnt + 1));
        }
        MC_RETURN_ERROR(json_dump(*it, out, ident, ident_cnt + 1));
    }
    MC_RETURN_ERROR(mc_fmt(out, "\n"));
    MC_RETURN_ERROR(dump_ident(out, ident, ident_cnt));
    MC_RETURN_ERROR(mc_fmt(out, "]"));

    return MCE_OK;
}

static MC_Error dump_kv_array(struct KvArray *arr, MC_Stream *out, MC_Str ident, unsigned ident_cnt){
    if(arr == NULL || arr->size == 0){
        return mc_fmt(out, "{}");
    }

    MC_RETURN_ERROR(mc_fmt(out, "{\n"));
    MC_RETURN_ERROR(dump_ident(out, ident, ident_cnt + 1));
    for(struct Kv *it = arr->kvs, *end = it + arr->size; it != end; it++){
        if(it != arr->kvs){
            MC_RETURN_ERROR(mc_fmt(out, ",\n"));
            MC_RETURN_ERROR(dump_ident(out, ident, ident_cnt + 1));
        }
        MC_RETURN_ERROR(dump_string(it->key, out));
        MC_RETURN_ERROR(mc_fmt(out, ": "));
        MC_RETURN_ERROR(json_dump(it->value, out, ident, ident_cnt + 1));
    }
    MC_RETURN_ERROR(mc_fmt(out, "\n"));
    MC_RETURN_ERROR(dump_ident(out, ident, ident_cnt));
    MC_RETURN_ERROR(mc_fmt(out, "}"));

    return MCE_OK;
}

static MC_Error dump_string(const struct String *string, MC_Stream *out){
    if(string == NULL){
        return mc_fmt(out, __STRING(""));
    }

    char buf[256];
    char *buf_cur = buf;
    char *buf_end = buf + sizeof(buf) - 6;

    *buf_cur++ = '\"';

    for(const char *ch = string->data, *end = ch + string->length; ch < end;){
        for(; ch < end && buf_cur < buf_end; ch++){
            switch (*ch){
            case '\\':
                strcpy(buf_cur, "\\\\");
                buf_cur += 2;
                break;
            case '\"':
                strcpy(buf_cur, "\\\"");
                buf_cur += 2;
                break;
            case '\t':
                strcpy(buf_cur, "\\t");
                buf_cur += 2;
                break;
            case '\n':
                strcpy(buf_cur, "\\n");
                buf_cur += 2;
                break;
            case '\b':
                strcpy(buf_cur, "\\b");
                buf_cur += 2;
                break;
            case '\f':
                strcpy(buf_cur, "\\f");
                buf_cur += 2;
                break;
            case '\0':
                strcpy(buf_cur, "\\0");
                buf_cur += 2;
                break;
            default:
                *buf_cur++ = *ch;
                break;
            }
        }

        if(ch == end){
            *buf_cur++ = '\"';
        }

        MC_RETURN_ERROR(mc_fmt(out, "%.*s", (int)(buf_cur- buf), buf));
        buf_cur = buf;
    }

    return MCE_OK;
}

static MC_Error string_fmt(MC_Alloc *alloc, struct String **ret_string, const char *fmt, va_list args){
    *ret_string = NULL;
    va_list args_cp;
    va_copy(args_cp, args);
    int len = vsnprintf(NULL, 0, fmt, args_cp);
    va_end(args_cp);

    struct String *string;
    MC_RETURN_ERROR(mc_alloc(alloc, sizeof(struct String) + len + 1, (void**)&string));

    string->length = len;
    vsnprintf(string->data, len + 1, fmt, args);
    *ret_string = string;
    return MCE_OK;
}

static MC_Error object_add(MC_Json *json, struct String *key, MC_Json *item){
    MC_RETURN_INVALID(json == NULL);
    if(json->type != MC_JSON_OBJECT){
        MC_RETURN_INVALID(json->type != MC_JSON_NULL);
        MC_RETURN_ERROR(mc_json_set_object(json));
    }

    assert(json->subtype == SUBTYPE_KEY_VALUE_ARRAY);

    if(json->as.kv_arr == NULL){
        MC_RETURN_ERROR(mc_alloc(json->alloc, sizeof(struct KvArray) + sizeof(struct Kv[INITIAL_LIST_SIZE]), (void**)&json->as.kv_arr));
        json->as.kv_arr->size = 0;
        json->as.kv_arr->capacity = INITIAL_LIST_SIZE;
    }
    else if(json->as.kv_arr->size == json->as.kv_arr->capacity){
        struct KvArray *new_array;
        size_t new_capacity = json->as.kv_arr->size * 2;
        MC_RETURN_ERROR(mc_alloc(json->alloc, sizeof(struct KvArray) + sizeof(struct Kv*[new_capacity]), (void**)&new_array));

        memcpy(new_array, json->as.arr, sizeof(struct KvArray) + sizeof(struct Kv*[json->as.arr->size]));
        new_array->capacity = new_capacity;
        mc_free(json->alloc, json->as.kv_arr);
        json->as.kv_arr = new_array;
    }

    if(item == NULL){
        MC_RETURN_ERROR(mc_json_new(json->alloc, &item));
    }

    for(struct Kv *kv = json->as.kv_arr->kvs, *end = json->as.kv_arr->kvs + json->as.kv_arr->size; kv != end; kv++){
        if(key->length == kv->key->length && strncmp(kv->key->data, key->data, key->length) == 0){
            mc_json_delete(&kv->value);
            kv->value = item;

            return MCE_OK;
        }
    }

    json->as.kv_arr->kvs[json->as.kv_arr->size++] = (struct Kv){
        .key = key,
        .value = item
    };

    return MCE_OK;
}
