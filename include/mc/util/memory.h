#ifndef MC_UTIL_MEMORY_H
#define MC_UTIL_MEMORY_H

#include <mc/error.h>

#include <stdalign.h>
#include <stdint.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdarg.h>

#define MC_ALIGN(TO, VALUE) (((VALUE) + (TO) - 1) & ~((TO) - 1))

static inline MC_Error mc_malloc_allv(void *ptr, size_t size, va_list args){
    if(ptr == NULL){
        return MCE_OK;
    }

    void *mem = *(void**)ptr  = malloc(size);
    if(!mem){
        return MCE_OUT_OF_MEMORY;
    }

    ptr = va_arg(args, void*);
    size = va_arg(args, size_t);

    MC_Error status = mc_malloc_allv(ptr, size, args);
    if(status != MCE_OK){
        free(mem);
    }

    return status;
}

static inline MC_Error mc_malloc_all(void *ptr, size_t size, ...){
    va_list args;
    va_start(args, size);
    MC_Error status = mc_malloc_allv(ptr, size, args);
    va_end(args);
    return status;
}

#endif // MC_UTIL_MEMORY_H
