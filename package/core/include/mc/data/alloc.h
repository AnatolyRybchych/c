#ifndef MC_DATA_ALLOC_H
#define MC_DATA_ALLOC_H

#include <mc/error.h>

#include <stddef.h>
#include <stdarg.h>

typedef struct MC_Alloc MC_Alloc;

MC_Error mc_alloc(MC_Alloc *allocator, size_t size, void **ret_ptr);
void mc_free(MC_Alloc *allocator, void *ptr);

MC_Error mc_alloc_allv(MC_Alloc *alloc, void *ptr, size_t size, va_list args);
MC_Error mc_alloc_all(MC_Alloc *alloc, void *ptr, size_t size, ...);

struct MC_Alloc{
    void *(*alloc)(MC_Alloc *this, size_t size);
    void (*free)(MC_Alloc *this, void *ptr);
};

extern MC_Alloc mc_alloc_malloc;
extern MC_Alloc mc_alloc_nomem;

#endif // MC_DATA_ALLOC_H
