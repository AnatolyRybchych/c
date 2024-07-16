#include <mc/graphics/di.h>

#include <malloc.h>

struct MC_Di{
    MC_Blend blend;
};

MC_Error mc_di_init(MC_Di **ret_di){
    MC_Di *di = malloc(sizeof(MC_Blend));
    if(di == NULL){
        return MCE_OUT_OF_MEMORY;
    }

    *di = (MC_Di){
        .blend = MC_BLEND_SET_SRC
    };

    *ret_di = di;
    return MCE_OK;
}

void mc_di_destroy(MC_Di *di){
    free(di);
}

void mc_di_clear(MC_Di *di, MC_DiBuffer *buffer, MC_AColor color){
    (void)di;

    for(MC_AColor *px = buffer->pixels; px < buffer->pixels + buffer->size.width * buffer->size.height; px++){
        *px = color;
    }
}