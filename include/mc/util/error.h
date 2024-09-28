#ifndef MC_UTIL_ERROR_H
#define MC_UTIL_ERROR_H

#include <mc/error.h>
#include <mc/os/file.h>

#define MC_ERROR_TRACES

#define MC_RETURN_INVALID(...) if(__VA_ARGS__) return MCE_INVALID_INPUT

MC_Error mc_error_from_errno(int err_no);

#ifdef MC_ERROR_TRACES
    #define MC_RETURN_ERROR(...) for(MC_Error __mc_return_error_status = (__VA_ARGS__); \
        __mc_return_error_status != MCE_OK && __mc_return_error_status != MCE_AGAIN;) \
            return (mc_fmt(MC_STDOUT, "%s:%i %s() %s -> %s\n", \
                __FILE__, __LINE__, __func__, #__VA_ARGS__, \
                mc_strerror(__mc_return_error_status))), __mc_return_error_status
#else
    #define MC_RETURN_ERROR(...) for(MC_Error __mc_return_error_status = (__VA_ARGS__); \
        __mc_return_error_status != MCE_OK && __mc_return_error_status != MCE_AGAIN;) \
            return __mc_return_error_status

#endif

#endif // MC_UTIL_ERROR_H
