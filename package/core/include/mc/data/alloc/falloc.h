#ifndef MC_DATA_FALLOC_H
#define MC_DATA_FALLOC_H

#include <mc/data/alloc.h>

typedef struct MC_Falloc MC_Falloc;

MC_Alloc *mc_fallok(MC_Falloc *fallback_alloc, size_t cnt, MC_Alloc **fallbacks);
MC_Alloc *mc_fallok_allocator(MC_Falloc *fallback_alloc);

struct MC_Falloc{
    MC_Alloc alloc;
    size_t fallbacks_cnt;
    MC_Alloc **fallbacks;
};

#endif // MC_DATA_FALLOC_H
