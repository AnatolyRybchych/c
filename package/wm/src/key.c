#include <mc/wm/key.h>

extern inline const char *mc_key_str(MC_Key key);

MC_Key mc_key_from_str(MC_Str str){
    str = mc_str_trim(str);
    if(MC_STR_LEN(str) >= 2 && str.beg[0] == '<' && str.end[-1] == '>'){
        str = mc_str_trim(MC_STR(str.beg + 1, str.end - 1));
    }

    #define MC_KEY(NAME, ...) if(mc_str_ci_equ(str, MC_STRC(#NAME))){ return MC_KEY_##NAME; }
    MC_ITER_KEYS()
    #undef MC_KEY

    return MC_KEY_UNKNOWN;
}
