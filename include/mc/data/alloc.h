#ifndef MC_DATA_ALLOC_H
#define MC_DATA_ALLOC_H

#include <stddef.h>
#include <mc/error.h>

typedef struct MC_Alloc MC_Alloc;

MC_Error mc_alloc(MC_Alloc *allocator, size_t size, void **ret_ptr);
void mc_free(MC_Alloc *allocator, void *ptr);

struct MC_Alloc{
    void *(*alloc)(MC_Alloc *this, size_t size);
    void (*free)(MC_Alloc *this, void *ptr);
};

extern MC_Alloc mc_alloc_malloc;
extern MC_Alloc mc_alloc_nomem;

#endif // MC_DATA_ALLOC_H
