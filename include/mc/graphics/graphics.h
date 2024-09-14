#ifndef MC_GRAPHICS_H
#define MC_GRAPHICS_H

#include <mc/geometry/size.h>
#include <mc/geometry/lina.h>
#include <mc/error.h>

#include <stddef.h>
#include <stdint.h>

typedef struct MC_Graphics MC_Graphics;
typedef struct MC_GBuffer MC_GBuffer;
typedef struct MC_GraphicsVtab MC_GraphicsVtab;

typedef struct MC_Color MC_Color;
typedef struct MC_AColor MC_AColor;

struct MC_TargetGraphics;

struct MC_Color{
    uint8_t r, g, b;
};

struct MC_AColor{
    uint8_t r, g, b, a;
};

struct MC_Stream;

MC_Error mc_graphics_init(MC_Graphics **g, const MC_GraphicsVtab *vtab, size_t ctx_size, const void *ctx);
void mc_graphics_destroy(MC_Graphics *g);

struct MC_TargetGraphics *mc_graphics_target(MC_Graphics *g);

MC_Error mc_graphics_begin(MC_Graphics *g);
MC_Error mc_graphics_end(MC_Graphics *g);

MC_Error mc_graphics_create_buffer(MC_Graphics *g, MC_GBuffer **buffer, MC_Size2U size_px);
void mc_graphics_destroy_buffer(MC_GBuffer *buffer);
MC_Error mc_graphics_select_buffer(MC_Graphics *g, MC_GBuffer *buffer);
MC_Error mc_graphics_write(MC_Graphics *g, MC_Vec2i pos, MC_Size2U size, MC_GBuffer *buffer, MC_Vec2i src_pos);
MC_Error mc_graphics_write_pixels(MC_Graphics *g, MC_Vec2i pos, MC_Size2U size, const MC_AColor pixels[size.height][size.width], MC_Vec2i src_pos);
MC_Error mc_graphics_read_pixels(MC_Graphics *g, MC_Vec2i pos, MC_Size2U size, MC_AColor pixels[size.height][size.width]);
MC_Error mc_graphics_get_size(MC_Graphics *g, MC_Size2U *size);

MC_Error mc_graphics_clear(MC_Graphics *g, MC_Color color);

MC_Error mc_graphics_dump_bmp(MC_Graphics *g, struct MC_Stream *stream);

inline uint8_t mc_u8_clamp(int val){
        if(val <= 0){
        return val;
    }

    if(val >= 0xFF){
        return 0xFF;
    }

    return val;
}

inline MC_Color mc_color(MC_AColor acolor){
    return (MC_Color){
        .r = acolor.r,
        .g = acolor.g,
        .b = acolor.b
    };
}

inline MC_AColor mc_acolor(MC_Color color, float alpha){
    return (MC_AColor){
        .a = alpha,
        .r = color.r,
        .g = color.g,
        .b = color.b
    };
}

inline MC_Color mc_color_lerp(MC_Color c1, MC_Color c2, float factor){
    return (MC_Color){
        .r = mc_lerpf(c1.r, c2.r, factor),
        .g = mc_lerpf(c1.g, c2.g, factor),
        .b = mc_lerpf(c1.b, c2.b, factor)
    };
}

inline MC_AColor mc_acolor_lerp(MC_AColor c1, MC_AColor c2, float factor){
    return mc_acolor(
        mc_color_lerp(mc_color(c1), mc_color(c2), factor),
        mc_lerpf(c1.a, c2.a, factor));
}

#endif // MC_GRAPHICS_H
