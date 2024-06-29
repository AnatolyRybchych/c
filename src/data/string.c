#include <mc/data/string.h>

#include <malloc.h>
#include <memory.h>

extern inline MC_Str mc_string_str(const MC_String *string);

MC_String *mc_string(MC_Str str){
    MC_String *res = malloc(sizeof(MC_String) + MC_STR_LEN(str) + 1);
    if(res){
        res->len = MC_STR_LEN(str);
        memcpy(res->data, str.beg, res->len);
        res->data[res->len] = '\0';
    }

    return res;
}

MC_String *mc_stringn(size_t len){
    MC_String *res = malloc(sizeof(MC_String) + len + 1);
    if(res){
        res->len = len;
        res->data[0] = '\0';
        res->data[res->len] = '\0';
    }

    return res;
}

MC_String *mc_string_fmtv(const char *fmt, va_list args){
    va_list args_cp;
    va_copy(args_cp, args);
    int len = vsnprintf(NULL, 0, fmt, args_cp);
    va_end(args_cp);

    MC_String *string = mc_stringn(len);
    if(string){
        vsnprintf(string->data, len, fmt, args);
    }

    return string;
}

MC_String *mc_string_fmt(const char *fmt, ...){
    va_list args;
    va_start(args, fmt);
    MC_String *res = mc_string_fmtv(fmt, args);
    va_end(args);
    return res;
}
