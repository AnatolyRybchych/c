#ifndef MC_DI_H
#define MC_DI_H

#include <stdint.h>
#include <mc/graphics/graphics.h>
#include <mc/error.h>

typedef struct MC_DiBuffer MC_DiBuffer;
typedef struct MC_Di MC_Di;
typedef unsigned MC_Blend;
enum MC_Blend{
    MC_BLEND_SET_SRC,
    MC_BLEND_SET_SRC_COLOR,
    MC_BLEND_SET_SRC_ALPHA,
};

MC_Error mc_di_init(MC_Di **di);
void mc_di_destroy(MC_Di *di);

void mc_di_clear(MC_Di *di, MC_DiBuffer *buffer, MC_AColor color);

struct MC_DiBuffer{
    MC_Size2U size;
    MC_AColor *pixels;
};

#endif // MC_DI_H
