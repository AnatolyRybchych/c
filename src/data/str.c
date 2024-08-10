#include <mc/data/str.h>

#include <string.h>

#define OPTIONAL_SET(DST, ...) if(DST) *(DST) = (__VA_ARGS__)

typedef struct Repeat{
    size_t min;
    size_t max;
} Repeat;

inline size_t mc_str_len(MC_Str str);
inline bool mc_starts_with(MC_Str str, MC_Str substr);
extern inline bool mc_str_empty(MC_Str str);

static bool match_from_startv(MC_Str str, MC_Str fmt, MC_Str *whole, va_list args);
static bool matchv(MC_Str str, MC_Str fmt, MC_Str *whole, va_list args);
static MC_Str parse_atom(MC_Str *str);
static Repeat parse_repeat(MC_Str *str);
static const char *match_atom(MC_Str str, MC_Str atom);

bool mc_str_matchv(MC_Str str, const char *fmt, MC_Str *whole, va_list args){
    return matchv(str, MC_STR(fmt, fmt + strlen(fmt)), whole, args);
}

bool mc_str_match(MC_Str str, const char *fmt, MC_Str *whole, ...){
    va_list args;
    va_start(args, whole);
    bool res = mc_str_matchv(str, fmt, whole, args);
    va_end(args);
    return res;
}

static bool match_from_startv(MC_Str str, MC_Str fmt, MC_Str *whole, va_list args){
    const char *cur = str.beg;

    while(fmt.beg != fmt.end){
        MC_Str atom = parse_atom(&str);
        Repeat repeat = parse_repeat(&str);

        for(int i = 0; i < repeat.max; i++){
            
        }
    }

    OPTIONAL_SET(whole, MC_STR(str.beg, cur));
    return true;
}

static bool match_from_start_to_endv(MC_Str str, MC_Str fmt, MC_Str *whole, va_list args){

}

static bool match_from_endv(MC_Str str, MC_Str fmt, MC_Str *whole, va_list args){

}

static bool matchv(MC_Str str, MC_Str fmt, MC_Str *whole, va_list args){
    if(mc_str_empty(fmt)){
        return match_from_startv(str, fmt, whole, args);
    }

    if(*fmt.beg == '^'){
        if(fmt.end[-1] == '$'){
            return match_from_start_to_endv(str, MC_STR(fmt.beg + 1, fmt.end - 1), whole, args);
        }

        return match_from_startv(str, MC_STR(fmt.beg + 1, fmt.end), whole, args);
    }

    if(fmt.end[-1] == '$'){
        return match_from_endv(str, MC_STR(fmt.beg, fmt.end - 1), whole, args);
    }

    for(va_list args_cur; str.beg != str.end; str.beg++){
        va_copy(args_cur, args);
        bool matches = match_from_startv(str, fmt, whole, args_cur);
        va_end(args_cur);

        if(matches){
            return true;
        }
    }

    return false;
}

static MC_Str parse_atom(MC_Str *str){
    MC_Str res = MC_STR(str->beg, str->beg + 1);
    str->beg++;
    return res;
}

static Repeat parse_repeat(MC_Str *str){
    if(mc_str_empty(*str) == 0){
        return (Repeat){.min = 1, .max = 1};
    }

    switch (*str->beg){
    case '*':
        str->beg++;
        return (Repeat){.min = 0, .max = ~0};;
    case '+':
        str->beg++;
        return (Repeat){.min = 1, .max = ~0};
    default:
        return (Repeat){.min = 1, .max = 1};
    }
}

static const char *match_atom(MC_Str str, MC_Str atom){
    if(mc_str_empty(atom)){
        return str.beg;
    }

    if(*atom.beg == *str.beg){
        return atom.beg + 1;
    }

    return NULL;
}

