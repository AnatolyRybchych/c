#ifndef MC_DATA_ALLOC_H
#define MC_DATA_ALLOC_H

#include <mc/error.h>

#include <stddef.h>
#include <stdarg.h>

typedef struct MC_Alloc MC_Alloc;

MC_Error mc_alloc(MC_Alloc *allocator, size_t size, void **ret_ptr);
void mc_free(MC_Alloc *allocator, void *ptr);

struct MC_Alloc{
    void *(*alloc)(MC_Alloc *this, size_t size);
    void (*free)(MC_Alloc *this, void *ptr);
};

static inline MC_Error mc_alloc_allv(MC_Alloc *alloc, void *ptr, size_t size, va_list args){
    if(ptr == NULL){
        return MCE_OK;
    }

    MC_RETURN_ERROR(mc_alloc(alloc, size, ptr));

    ptr = va_arg(args, void*);
    size = va_arg(args, size_t);

    MC_Error status = mc_alloc_allv(alloc, ptr, size, args);
    if(status != MCE_OK){
        mc_free(alloc, *(void**)ptr);
    }

    return status;
}

static inline MC_Error mc_alloc_all(MC_Alloc *alloc, void *ptr, size_t size, ...){
    va_list args;
    va_start(args, size);
    MC_Error status = mc_alloc_allv(alloc, ptr, size, args);
    va_end(args);
    return status;
}

extern MC_Alloc mc_alloc_malloc;
extern MC_Alloc mc_alloc_nomem;

#endif // MC_DATA_ALLOC_H
