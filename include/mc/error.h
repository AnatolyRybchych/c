#ifndef MC_ERROR_H
#define MC_ERROR_H

typedef unsigned MC_Error;

#define MC_ITER_ERRORS() \
    MC_ERROR(OK) \
    MC_ERROR(INVALID_INPUT) \
    MC_ERROR(NOT_IMPLEMENTED) \
    MC_ERROR(NOT_SUPPORTED) \
    MC_ERROR(NOT_FOUND) \
    MC_ERROR(OUT_OF_MEMORY) \
    MC_ERROR(BLOCK) \
    MC_ERROR(BUSY) \
    MC_ERROR(OVERFLOW) \

enum MC_Error{
    #define MC_ERROR(ERROR, ...) MCE_##ERROR,
    MC_ITER_ERRORS()
    #undef MC_ERROR
};

inline const char *mc_strerror(MC_Error err){
    switch (err){
    #define MC_ERROR(ERROR, ...) case MCE_##ERROR: return #ERROR;
    MC_ITER_ERRORS()
    #undef MC_ERROR
    default: return (const char *)0;
    }
}

#endif // MC_ERROR_H
