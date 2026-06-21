#include <mc/data/alloc.h>

#include <mc/util/error.h>

#include <memory.h>
#include <malloc.h>

static void *_malloc(MC_Alloc *alloc, size_t size);
static void _free(MC_Alloc *alloc, void *ptr);
static void *_realloc(MC_Alloc *alloc, void *ptr, size_t size);

MC_Alloc mc_alloc_malloc = {
    .alloc = _malloc,
    .free = _free,
    .realloc = _realloc
};

MC_Alloc mc_alloc_nomem = {
    .alloc = NULL,
    .free = NULL
};

static MC_Alloc *default_alloc = &mc_alloc_malloc;

MC_Error mc_alloc(MC_Alloc *allocator, size_t size, void **ret_ptr){
    if(allocator == NULL){
        allocator = default_alloc;
    }

    if(allocator->alloc){
        *ret_ptr = allocator->alloc(allocator, size);
    }
    else if(allocator->realloc){
        *ret_ptr = allocator->realloc(allocator, NULL, size);
    }
    else{
        *ret_ptr = NULL;
    }

    return *ret_ptr ? MCE_OK : MCE_OUT_OF_MEMORY;
}

MC_Error mc_realloc(MC_Alloc *allocator, void **ptr, size_t size){
    if(allocator == NULL){
        allocator = default_alloc;
    }

    if(allocator->realloc == NULL){
        return MCE_NOT_SUPPORTED;
    }

    void *resized = allocator->realloc(allocator, *ptr, size);
    if(size != 0 && resized == NULL){
        return MCE_OUT_OF_MEMORY;
    }

    *ptr = resized;
    return MCE_OK;
}

void mc_free(MC_Alloc *allocator, void *ptr){
    if(allocator == NULL){
        allocator = default_alloc;
    }

    if(allocator->free){
        allocator->free(allocator, ptr);
    }
}

MC_Error mc_alloc_allv(MC_Alloc *alloc, void *ptr, size_t size, va_list args){
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

MC_Error mc_alloc_all(MC_Alloc *alloc, void *ptr, size_t size, ...){
    va_list args;
    va_start(args, size);
    MC_Error status = mc_alloc_allv(alloc, ptr, size, args);
    va_end(args);
    return status;
}

static void *_malloc(MC_Alloc *alloc, size_t size){
    (void)alloc;
    return malloc(size);
}

static void _free(MC_Alloc *alloc, void *ptr){
    (void)alloc;
    free(ptr);
}

static void *_realloc(MC_Alloc *alloc, void *ptr, size_t size){
    (void)alloc;
    return realloc(ptr, size);
}
