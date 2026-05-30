#ifndef MC_DATA_STR_H
#define MC_DATA_STR_H

#include <mc/util/minmax.h>

#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

typedef struct MC_Str MC_Str;

#define MC_STR_LEN(STR) ((STR).end - (STR).beg)
#define MC_STR(BEG, END) ((MC_Str){.beg = BEG, .end = END})
#define MC_STRC(CSTR) MC_STR("" CSTR, CSTR + sizeof(CSTR) - 1)

struct MC_Str{
    const char *beg;
    const char *end;
};

/*
    EXPERIMENTAL
    INCOMPLETE
    USE ONLY WITH SMALL INPUT
    THE SYNTAX MAY CHANGE

    Character set "[]":
        Character set is designed to work without escapings and to not have invalid syntax possible

        -: ranges
            if '-' is placed at the beginig or ending of the set, its treated as a single character
            if '-' is placed after other '-', the second component of the range, its treated as character '-'
            the range bounds are commutative, "[a-b]" is the same as "[b-a]"
            "[+--]" - matches '+', ',', and '-' (ascii 0x2b - 0x2d)
            "[--+]" - matches '+', ',', and '-' (ascii 0x2d - 0x2b)
            "[-a]" - matches '-' and 'a'
            "[a-]" - matches '-' and 'a'
            "[^-]" - matches everything but '-'
            "[-]"  - matches only '-'
            "[a-b]"  - inclusively matches all the characters from 'a' to 'b'

        ^: inverse set
            if '^' is placed at the beginig of the set and its not the only item in the set then the set provides an inverse match for this set
            "[^a]" - matches everything but 'a'
            "[a^]" - matches 'a' and '^'
            "[^]" - matches only '^'

        [: opening bracket
            opening bracket is matched as regular character inside of the set
            "[[]" - matches '['
            "[[a]" - matches '[' and 'a'

        ]: closing bracket
            closing bracket can only be placed as the last element of the set
            "[]]" - matches ']'
            "[]-]" - matches ']', and '-'
            "[-]]" - matches ']', and '-'
            "[[-]]" - matches '[', ']', and '\\' (ascii 0x5b - 0x5d)
            "[^[-]]" - matches everything but '[', ']', and '\\' (ascii 0x5b - 0x5d)

        all other characters insidide the set are treated as the single character range
            "[a]" - matches 'a' (same as "[a-a]")

        if the range does not have a closing bracket, its not treated as a set
            "[abc" - matches "[abc" sequence

        multiple constructions inside the range can be combined into a sequence which matches any of the elements of this sequence
            "[a-zA-Z_-]" - matches any alphabetic character, '-', and '_'
            "[^a-zA-Z_-]" - matches everythin except alphabetic character, '-', and '_'
*/
bool mc_str_matchv(MC_Str str, const char *fmt, MC_Str *whole, va_list args);
bool mc_str_match(MC_Str str, const char *fmt, MC_Str *whole, ...);

inline MC_Str mc_strc(const char *str) {
    return str ? MC_STR(str, str + strlen(str)) : MC_STR(NULL, NULL);
}

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

inline MC_Str mc_str_hex_toull(MC_Str str, uint64_t *val) {
    for(*val = 0; mc_str_len(str) && isxdigit(*str.beg); str.beg++){
        *val *= 16;
        if(*str.beg >= 'a' && *str.beg <= 'f') {
            *val += *str.beg - 'a' + 10; 
        }
        else if(*str.beg >= 'A' && *str.beg <= 'F') {
            *val += *str.beg - 'A' + 10; 
        }
        else {
            *val += *str.beg - '0';
        }
    }

    return str;
}

inline bool mc_str_starts_with(MC_Str str, MC_Str substr){
    if(mc_str_len(substr) > mc_str_len(str)) {
        return false;
    }

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

inline bool mc_str_ends_with(MC_Str str, MC_Str substr){
    if(mc_str_len(substr) > mc_str_len(str)) {
        return false;
    }

    while(substr.beg != substr.end){
        if(str.beg == str.end){
            return false;
        }

        if(*str.end-- != *substr.end--){
            return false;
        }
    }

    return true;
}

inline MC_Str mc_str_pop_front(MC_Str *str, size_t cnt) {
    size_t to_pop = MIN(mc_str_len(*str), cnt);
    MC_Str result = MC_STR(str->beg, str->beg + to_pop);
    str->beg += to_pop;
    return result;
}

inline MC_Str mc_str_pop_back(MC_Str *str, size_t cnt) {
    size_t to_pop = MIN(mc_str_len(*str), cnt);
    MC_Str result = MC_STR(str->end - to_pop, str->end);
    str->end -= to_pop;
    return result;
}

inline bool mc_str_equ(MC_Str str, MC_Str other) {
    return mc_str_len(str) == mc_str_len(other)
        && mc_str_starts_with(str, other);
}

inline MC_Str mc_str_substr(MC_Str str, MC_Str substr) {
    for(const char *c = str.beg; c != str.end; c++) {
        if(mc_str_starts_with(MC_STR(c, str.end), substr)) {
            return MC_STR(c, c + mc_str_len(substr));
        }
    }

    return MC_STR(str.end, str.end);
}

inline bool mc_str_empty(MC_Str str){
    return mc_str_len(str) == 0;
}

#endif // MC_DATA_STR_H
