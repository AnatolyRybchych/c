#ifndef MC_SIGN_H
#define MC_SIGN_H

typedef int MC_Sign;
enum MC_Sign{
    MC_SIGN_LESS = -1,
    MC_SIGN_EQUALS = 0,
    MC_SIGN_GREATER = 1,
};

inline MC_Sign mc_sign(int value){
    if(value == 0) return MC_SIGN_EQUALS;
    if(value > 0) return MC_SIGN_GREATER;
    else return MC_SIGN_LESS;
}

#endif // MC_SIGN_H
