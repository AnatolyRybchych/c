#ifndef MC_ERROR_H
#define MC_ERROR_H

typedef unsigned MC_Error;

#define MC_ITER_ERRORS() \
    MC_ERROR(OK) \
    MC_ERROR(UNKNOWN) \
    MC_ERROR(INVALID_INPUT) \
    MC_ERROR(NOT_IMPLEMENTED) \
    MC_ERROR(NOT_SUPPORTED) \
    MC_ERROR(NOT_FOUND) \
    MC_ERROR(OUT_OF_MEMORY) \
    MC_ERROR(BLOCK) \
    MC_ERROR(BUSY) \
    MC_ERROR(OVERFLOW) \
    MC_ERROR(TIMEOUT) \
    MC_ERROR(AGAIN) \
    MC_ERROR(CONNECTION_REFUSED) \
    MC_ERROR(NOT_PERMITTED) \

#define MC_RETURN_ERROR(...) for(MC_Error __mc_return_error_status = (__VA_ARGS__); \
    __mc_return_error_status != MCE_OK && __mc_return_error_status != MCE_AGAIN;) \
        return __mc_return_error_status
#define MC_RETURN_INVALID(...) if(__VA_ARGS__) return MCE_INVALID_INPUT

enum MC_Error{
    #define MC_ERROR(ERROR, ...) MCE_##ERROR,
    MC_ITER_ERRORS()
    #undef MC_ERROR
};

MC_Error mc_error_from_errno(int err_no);

inline const char *mc_strerror(MC_Error err){
    switch (err){
    #define MC_ERROR(ERROR, ...) case MCE_##ERROR: return #ERROR;
    MC_ITER_ERRORS()
    #undef MC_ERROR
    default: return (const char *)0;
    }
}


#endif // MC_ERROR_H
