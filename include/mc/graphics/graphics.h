#ifndef MC_GRAPHICS_H
#define MC_GRAPHICS_H

#include <mc/geometry.h>
#include <mc/error.h>

#include <stddef.h>

typedef struct MC_Graphics MC_Graphics;
typedef struct MC_GraphicsVtab MC_GraphicsVtab;

typedef struct MC_Color MC_Color;
typedef struct MC_AColor MC_AColor;

struct MC_TargetGraphics;

struct MC_Color{
    float r, g, b;
};

struct MC_AColor{
    MC_Color color;
    float alpha;
};

MC_Error mc_graphics_init(MC_Graphics **g, const MC_GraphicsVtab *vtab, size_t ctx_size, const void *ctx);
void mc_graphics_destroy(MC_Graphics *g);

struct MC_TargetGraphics *mc_graphics_target(MC_Graphics *g);

MC_Error mc_graphics_begin(MC_Graphics *g);
MC_Error mc_graphics_end(MC_Graphics *g);
MC_Error mc_graphics_clear(MC_Graphics *g, MC_Color color);


#endif // MC_GRAPHICS_H
