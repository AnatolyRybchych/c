#include <mc/graphics/graphics.h>
#include <mc/graphics/target.h>

#include <malloc.h>
#include <stdalign.h>
#include <stdint.h>
#include <memory.h>

struct MC_Graphics{
    MC_GraphicsVtab vtab;
    MC_TargetGraphics *target;
    alignas(void*) uint8_t data[];
};

MC_Error mc_graphics_init(MC_Graphics **ret_g, const MC_GraphicsVtab *vtab, size_t ctx_size, const void *ctx){
    MC_Graphics *g = malloc(sizeof(MC_Graphics) + ctx_size);
    if(g == NULL){
        return MCE_OUT_OF_MEMORY;
    }

    if(ctx != NULL){
        memcpy(g->data, ctx, ctx_size);
    }

    g->vtab = *vtab;
    g->target = (MC_TargetGraphics*)g->data;

    *ret_g = g;
    return MCE_OK;
}

void mc_graphics_destroy(MC_Graphics *g){
    g->vtab.destroy(g->target);
    free(g);
}

MC_Error mc_graphics_begin(MC_Graphics *g){
    if(g->vtab.begin == NULL){
        return MCE_OK;
    }

    return g->vtab.begin(g->target);
}

MC_Error mc_graphics_end(MC_Graphics *g){
    if(g->vtab.end == NULL){
        return MCE_OK;
    }

    return g->vtab.end(g->target);
}

struct MC_TargetGraphics *mc_graphics_target(MC_Graphics *g){
    return g->target;
}

MC_Error mc_graphics_clear(MC_Graphics *g, MC_Color color){
    if(g->vtab.clear){
        return g->vtab.clear(g->target, color);
    }

    return MCE_NOT_SUPPORTED;
}
