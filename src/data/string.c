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
