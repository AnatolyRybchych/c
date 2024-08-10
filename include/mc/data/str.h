#ifndef MC_DATA_STR_H
#define MC_DATA_STR_H

#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>

typedef struct MC_Str MC_Str;

#define MC_STR_LEN(STR) ((STR).end - (STR).beg)
#define MC_STR(BEG, END) ((MC_Str){.beg = BEG, .end = END})
#define MC_STRC(CSTR) MC_STR("" CSTR, CSTR + sizeof(CSTR))


bool mc_str_matchv(MC_Str str, const char *fmt, MC_Str *whole, va_list args);
bool mc_str_match(MC_Str str, const char *fmt, MC_Str *whole, ...);

struct MC_Str{
    const char *beg;
    const char *end;
};

inline size_t mc_str_len(MC_Str str){
    return str.end - str.beg;
}

inline bool mc_starts_with(MC_Str str, MC_Str substr){
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
