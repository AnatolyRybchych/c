#ifndef MC_DATA_STRING_H
#define MC_DATA_STRING_H

#include <mc/data/str.h>
#include <mc/data/alloc.h>
#include <mc/error.h>

#include <stddef.h>
#include <stdarg.h>

typedef struct MC_String MC_String;

MC_Error mc_string(MC_Alloc *alloc, MC_String **string, MC_Str str);
MC_Error mc_stringn(MC_Alloc *alloc, MC_String **string, size_t len);
MC_Error mc_string_fmtv(MC_Alloc *alloc, MC_String **string, const char *fmt, va_list args);
MC_Error mc_string_fmt(MC_Alloc *alloc, MC_String **string, const char *fmt, ...);

struct MC_String{
    size_t len;
    char data[];
};

inline MC_Str mc_string_str(const MC_String *string){
    return string 
        ? MC_STR(string->data, string->data + string->len)
        : MC_STRC("");
}

#endif // MC_DATA_STRING_H
