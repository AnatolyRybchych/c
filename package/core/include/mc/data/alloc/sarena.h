#ifndef MC_DATA_ALLOC_BUF_H
#define MC_DATA_ALLOC_BUF_H

#include <mc/data/alloc.h>

typedef struct MC_Sarena MC_Sarena;

MC_Alloc *mc_sarena(MC_Sarena *sarena, size_t bufsz, void *buf);
MC_Alloc *mc_sarena_allocator(MC_Sarena *sarena);
void mc_sarena_reset(MC_Sarena *sarena);

struct MC_Sarena{
    MC_Alloc alloc;
    void *buf;
    void *buf_cur;
    void *buf_end;
};


#endif // MC_DATA_ALLOC_BUF_H
