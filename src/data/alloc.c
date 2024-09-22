#include <mc/data/alloc.h>

#include <memory.h>
#include <malloc.h>

static void *_malloc(MC_Alloc *alloc, size_t size);
static void _free(MC_Alloc *alloc, void *ptr);

MC_Alloc mc_alloc_malloc = {
    .alloc = _malloc,
    .free = _free
};

MC_Alloc mc_alloc_nomem = {
    .alloc = NULL,
    .free = NULL
};

MC_Error mc_alloc(MC_Alloc *allocator, size_t size, void **ret_ptr){
    if(allocator == NULL){
        *ret_ptr = malloc(size);
    }
    else{
        *ret_ptr = allocator->alloc ? allocator->alloc(allocator, size) : NULL;
    }

    return *ret_ptr ? MCE_OK : MCE_OUT_OF_MEMORY;
}

void mc_free(MC_Alloc *allocator, void *ptr){
    if(allocator && allocator->free){
        allocator->free(allocator, ptr);
    }
}

static void *_malloc(MC_Alloc *alloc, size_t size){
    (void)alloc;
    return malloc(size);
}

static void _free(MC_Alloc *alloc, void *ptr){
    (void)alloc;
    free(ptr);
}
