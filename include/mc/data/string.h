#ifndef MC_DATA_STRING_H
#define MC_DATA_STRING_H

#include <stddef.h>
#include <mc/data/str.h>

typedef struct MC_String MC_String;

MC_String *mc_string(MC_Str str);

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
