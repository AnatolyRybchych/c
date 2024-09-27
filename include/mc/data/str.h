#ifndef MC_DATA_STR_H
#define MC_DATA_STR_H

#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <ctype.h>

typedef struct MC_Str MC_Str;

#define MC_STR_LEN(STR) ((STR).end - (STR).beg)
#define MC_STR(BEG, END) ((MC_Str){.beg = BEG, .end = END})
#define MC_STRC(CSTR) MC_STR("" CSTR, CSTR + sizeof(CSTR) - 1)


bool mc_str_matchv(MC_Str str, const char *fmt, MC_Str *whole, va_list args);
bool mc_str_match(MC_Str str, const char *fmt, MC_Str *whole, ...);

struct MC_Str{
    const char *beg;
    const char *end;
};

inline size_t mc_str_len(MC_Str str){
    return str.end - str.beg;
}

inline MC_Str mc_str_ltrim(MC_Str str){
    while (str.beg != str.end && isspace(*str.beg)){
        str.beg++;
    }

    return str;
}

inline MC_Str mc_str_rtrim(MC_Str str){
    while (str.beg != str.end && isspace(str.end[-1])){
        str.end--;
    }

    return str;
}

inline MC_Str mc_str_trim(MC_Str str){
    return mc_str_ltrim(mc_str_rtrim(str));
}

inline MC_Str mc_str_toull(MC_Str str, uint64_t *val){
    for(*val = 0; mc_str_len(str) && isdigit(*str.beg); str.beg++){
        *val *= 10;
        *val += *str.beg - '0';
    }

    return str;
}

inline bool mc_str_starts_with(MC_Str str, MC_Str substr){
    while(substr.beg != substr.end){
        if(str.beg == str.end){
            return false;
        }

        if(*str.beg++ != *substr.beg++){
            return false;
        }
    }

    return true;
}

inline bool mc_str_empty(MC_Str str){
    return mc_str_len(str) == 0;
}

#endif // MC_DATA_STR_H
