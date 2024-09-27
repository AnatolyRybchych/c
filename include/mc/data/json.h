#ifndef MC_DATA_JSON_H
#define MC_DATA_JSON_H

#include <mc/error.h>
#include <mc/data/alloc.h>
#include <mc/data/str.h>
#include <mc/io/stream.h>

#include <stdbool.h>
#include <stdint.h>

#define MC_JSON_LIST_LOAD(LIST, ...) mc_json_list_load((LIST), MC_STRC(#__VA_ARGS__))

typedef struct MC_Json MC_Json;

typedef unsigned MC_JsonType;
enum MC_JsonType{
    MC_JSON_NULL,
    MC_JSON_BOOL,
    MC_JSON_NUMBER,
    MC_JSON_STRING,
    MC_JSON_LIST,
    MC_JSON_OBJECT,
};

MC_Error mc_json_new(MC_Alloc *alloc, MC_Json **json);
void mc_json_delete(MC_Json **json);

MC_JsonType mc_json_type(MC_Json *json);
size_t mc_json_length(MC_Json *json);

MC_Error mc_json_set_bool(MC_Json *json, bool value);
MC_Error mc_json_set_u64(MC_Json *json, uint64_t value);
MC_Error mc_json_set_i64(MC_Json *json, int64_t value);
MC_Error mc_json_set_lf(MC_Json *json, double value);
MC_Error mc_json_set_string(MC_Json *json, MC_Str str);
MC_Error mc_json_set_stringf(MC_Json *json, const char *fmt, ...);
MC_Error mc_json_set_null(MC_Json *json);
MC_Error mc_json_set_list(MC_Json *json);
MC_Error mc_json_set_object(MC_Json *json);

MC_Error mc_json_list_add_new(MC_Json *json, MC_Json **item);
MC_Error mc_json_list_add(MC_Json *json, MC_Json *item);
MC_Error mc_json_list_load(MC_Json *json, MC_Str str);
MC_Error mc_json_object_add(MC_Json *json, MC_Json *item, const char *key_fmt, ...);
MC_Error mc_json_object_addv(MC_Json *json, MC_Json *item, const char *key_fmt, va_list args);
MC_Error mc_json_object_add_new(MC_Json *json, MC_Json **item, const char *key_fmt, ...);

MC_Error mc_json_dump(MC_Json *json, MC_Stream *out);
MC_Error mc_json_loads(MC_Alloc *alloc, MC_Json **json, MC_Str str);

#endif // MC_DATA_JSON_H
