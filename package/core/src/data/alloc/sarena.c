#include <mc/data/alloc/sarena.h>

#include <mc/util/memory.h>

#include <stdint.h>

static void *_alloc(MC_Alloc *this, size_t size);

MC_Alloc *mc_sarena(MC_Sarena *sarena, size_t bufsz, void *buf){
    uint8_t *aligned_buf = (void*)mc_align(alignof(max_align_t), (size_t)buf);
    if((int)bufsz < aligned_buf - (uint8_t*)buf){
        buf = NULL;
        bufsz = 0;
    }
    else{
        bufsz -= aligned_buf - (uint8_t*)buf;
        buf = aligned_buf;
    }

    *sarena = (MC_Sarena){
        .alloc = {
            .alloc = _alloc,
            .free = NULL,
        },
        .buf = buf,
        .buf_cur = buf,
        .buf_end = aligned_buf + bufsz
    };

    return mc_sarena_allocator(sarena);
}

MC_Alloc *mc_sarena_allocator(MC_Sarena *sarena){
    return &sarena->alloc;
}

void mc_sarena_reset(MC_Sarena *sarena){
    sarena->buf_cur = sarena->buf;
}

static void *_alloc(MC_Alloc *this, size_t size){
    size = mc_align(alignof(max_align_t), size);

    MC_Sarena *sarena = (MC_Sarena*)this;
    if((uint8_t*)sarena->buf_cur + size > (uint8_t*)sarena->buf_end){
        return NULL;
    }

    void *ptr = sarena->buf_cur;
    sarena->buf_cur = (uint8_t*)sarena->buf_cur + size;
    return ptr;
}
