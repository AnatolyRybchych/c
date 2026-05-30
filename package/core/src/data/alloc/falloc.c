#include <mc/data/alloc/falloc.h>

#include <mc/util/memory.h>

#define HDR_SIZE  MC_ALIGN(sizeof(void*), sizeof(Hdr))

static void *_alloc(MC_Alloc *this, size_t size);
static void _free(MC_Alloc *this, void *ptr);

typedef struct Hdr Hdr;
struct Hdr{
    size_t alloc_idx;
    void *payload[];
};

MC_Alloc *mc_fallok(MC_Falloc *fallback_alloc, size_t cnt, MC_Alloc **fallbacks){
    *fallback_alloc = (MC_Falloc){
        .alloc = {
            .alloc = _alloc,
            .free = _free
        },
        .fallbacks_cnt = cnt,
        .fallbacks = fallbacks
    };

    return mc_fallok_allocator(fallback_alloc);
}

MC_Alloc *mc_fallok_allocator(MC_Falloc *fallback_alloc){
    return &fallback_alloc->alloc;
}

static void *_alloc(MC_Alloc *this, size_t size){
    MC_Falloc *falloc = (MC_Falloc*)this;

    size = MC_ALIGN(sizeof(void*), size) + HDR_SIZE;

    Hdr *hdr;
    for(size_t idx = 0; idx < falloc->fallbacks_cnt; idx++){
        if(mc_alloc(falloc->fallbacks[idx], size, (void**)&hdr) != MCE_OK)
            continue;
        
        hdr->alloc_idx = idx;
        return hdr->payload;
    }

    return NULL;
}

static void _free(MC_Alloc *this, void *ptr){
    MC_Falloc *falloc = (MC_Falloc*)this;

    Hdr *hdr = (Hdr*)ptr - 1;
    mc_free(falloc->fallbacks[hdr->alloc_idx], ptr);
}
