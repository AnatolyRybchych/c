#include <mc/data/string.h>

#include <mc/util/error.h>

#include <memory.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

extern inline MC_Str mc_string_str(const MC_String *string);

MC_Error mc_string(MC_Alloc *alloc, MC_String **out_string, MC_Str str){
    MC_RETURN_ERROR(mc_stringn(alloc, out_string, MC_STR_LEN(str)));

    MC_String *res = *out_string;
    memcpy(res->data, str.beg, res->len);

    return MCE_OK;
}

MC_Error mc_stringn(MC_Alloc *alloc, MC_String **out_string, size_t len){
    *out_string = NULL;
    MC_RETURN_ERROR(mc_alloc(alloc, sizeof(MC_String) + len + 1, (void**)out_string));

    MC_String *res = *out_string;
    res->len = len;
    res->data[0] = '\0';
    res->data[res->len] = '\0';

    return MCE_OK;
}

MC_Error mc_string_fmtv(MC_Alloc *alloc, MC_String **out_string, const char *fmt, va_list args){
    va_list args_cp;
    va_copy(args_cp, args);
    int len = vsnprintf(NULL, 0, fmt, args_cp);
    va_end(args_cp);

    MC_RETURN_ERROR(mc_stringn(alloc, out_string, len));
    MC_String *string = *out_string;
    vsnprintf(string->data, len, fmt, args);

    return MCE_OK;
}

MC_Error mc_string_fmt(MC_Alloc *alloc, MC_String **out_string, const char *fmt, ...){
    va_list args;
    va_start(args, fmt);
    MC_Error status = mc_string_fmtv(alloc, out_string, fmt, args);
    va_end(args);
    return status;
}
