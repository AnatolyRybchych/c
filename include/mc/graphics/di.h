#ifndef MC_DI_H
#define MC_DI_H

#include <mc/graphics/graphics.h>
#include <mc/geometry/size.h>
#include <mc/geometry/rect.h>
#include <mc/geometry/bezier.h>
#include <mc/error.h>

#include <stdint.h>
#include <stdbool.h>

typedef struct MC_DiBuffer MC_DiBuffer;
typedef struct MC_DiShape MC_DiShape;
typedef struct MC_Di MC_Di;

MC_Error mc_di_init(MC_Di **di);
void mc_di_destroy(MC_Di *di);

void mc_di_clear(MC_Di *di, MC_DiBuffer *buffer, MC_AColor color);

MC_Error mc_di_shape_create(MC_Di *di, MC_DiShape **shape, MC_Size2U size);
void mc_di_shape_delete(MC_Di *di, MC_DiShape *shape);

MC_Error mc_di_shape_circle(MC_Di *di, MC_DiShape *shape, MC_Vec2f pos, float radius);
MC_Error mc_di_shape_line(MC_Di *di, MC_DiShape *shape, MC_Vec2f p1, MC_Vec2f p2, float thikness);
MC_Error mc_di_shape_curve(MC_Di *di, MC_DiShape *shape, MC_Vec2f beg, size_t n, const MC_SemiBezier4f curve[n], float thikness);

MC_Error mc_di_fill(MC_Di *di, MC_DiBuffer *buffer, const MC_DiShape *shape, MC_Rect2IU dst, MC_AColor fill_color);

struct MC_DiBuffer{
    MC_Size2U size;
    MC_AColor *pixels;
};

struct MC_DiShape{
    MC_Size2U size;

    // each pixel represents the magnitude of the vector from nearest shape border to the pixel position
    // i.e if camp(pixel, 0, 1) > 0 then the pixel is over the shape
    float pixels[];
};

inline MC_AColor mc_di_blend(MC_AColor dst, MC_AColor src){
    uint8_t dfac = 255 - dst.a;
    uint8_t sfac = src.a;

    return (MC_AColor){
        .a = mc_u8_clamp(255 - ((255 - dst.a) / 2 + (255 - src.a) / 2)),
        .r = mc_u8_clamp((dst.r * dfac + sfac * src.r) / 255),
        .g = mc_u8_clamp((dst.g * dfac + sfac * src.g) / 255),
        .b = mc_u8_clamp((dst.b * dfac + sfac * src.b) / 255)
    };
}

inline bool mc_di_hoverover(const MC_DiBuffer *buf, MC_Vec2i pos){
    return !(pos.x < 0 || pos.y < 0
        || pos.x >= (int)buf->size.width || pos.y > (int)buf->size.height);
}

inline void mc_di_setpx_unsafe(MC_DiBuffer *buf, MC_Vec2i pos, MC_AColor color){
    MC_AColor (*pixels)[buf->size.height][buf->size.width] = (void*)buf->pixels;
    (*pixels)[pos.y][pos.x] = color;
}

inline void mc_di_setpx(MC_DiBuffer *buf, MC_Vec2i pos, MC_AColor color){
    if(mc_di_hoverover(buf, pos)){
        mc_di_setpx_unsafe(buf, pos, color);
    }
}

inline void mc_di_drawpx_unsafe(MC_DiBuffer *buf, MC_Vec2i pos, MC_AColor color){
    MC_AColor (*pixels)[buf->size.height][buf->size.width] = (void*)buf->pixels;
    mc_di_setpx_unsafe(buf, pos, mc_di_blend((*pixels)[pos.y][pos.x], color));
}

inline void mc_di_drawpx(MC_DiBuffer *buf, MC_Vec2i pos, MC_AColor color){
    if(mc_di_hoverover(buf, pos)){
        mc_di_drawpx_unsafe(buf, pos, color);
    }
}

inline MC_AColor mc_di_getpx(const MC_DiBuffer *buf, MC_Vec2i pos){
    pos = mc_vec2i_clamp(pos, mc_vec2i(0, 0), mc_vec2i(buf->size.width - 1, buf->size.height - 1));
    MC_AColor (*pixels)[buf->size.height][buf->size.width] = (void*)buf->pixels;
    return (*pixels)[pos.y][pos.x];
}

inline float mc_shape_getpx(const MC_DiShape *shape, MC_Vec2i pos){
    if(pos.x < 0 || pos.y < 0 || pos.x >= (int)shape->size.width || pos.y >= (int)shape->size.height)
        return 0;

    float (*pixels)[shape->size.height][shape->size.width] = (void*)shape->pixels;
    return (*pixels)[pos.y][pos.x];
}

/// @param pos є [0; 1]
inline float mc_shape_get_nearest(const MC_DiShape *shape, MC_Vec2f pos){
    pos = mc_vec2f_clamp(pos, mc_vec2f(0, 0), mc_vec2f(1, 1));
    MC_Vec2i ipos = mc_vec2i(pos.y * (shape->size.height - 0.5), pos.x * (shape->size.width - 0.5));

    float (*pixels)[shape->size.height][shape->size.width] = (void*)shape->pixels;
    return (*pixels)[ipos.y][ipos.x];
}

/// @param pos є [0; 1]
inline float mc_shape_get_linear(const MC_DiShape *shape, MC_Vec2f pos){
    MC_Vec2f absolute = mc_vec2f_mul(pos, mc_vec2f(shape->size.width - 0.5, shape->size.height - 0.5));
    MC_Vec2i iabsolute = mc_vec2i(absolute.x, absolute.y);

    MC_Vec2f displacement = mc_vec2f(absolute.x - iabsolute.x, absolute.y - iabsolute.y);

    MC_Vec2i idisplacement = mc_vec2i(
        displacement.x < 0.5 ? -1 : 1,
        displacement.y < 0.5 ? -1 : 1
    );

    displacement = mc_vec2f(
        fabsf(displacement.x - 0.5f),
        fabsf(displacement.y - 0.5f)
    );

    float px00 = mc_shape_getpx(shape, mc_vec2i(iabsolute.x, iabsolute.y));
    float px01 = mc_shape_getpx(shape, mc_vec2i(iabsolute.x, iabsolute.y + idisplacement.y));
    float px10 = mc_shape_getpx(shape, mc_vec2i(iabsolute.x + idisplacement.x, iabsolute.y));
    float px11 = mc_shape_getpx(shape, mc_vec2i(iabsolute.x + idisplacement.x, iabsolute.y + idisplacement.y));

    return mc_lerpf(
        mc_lerpf(px00, px01, displacement.y),
        mc_lerpf(px10, px11, displacement.y),
        displacement.x
    );
}

#endif // MC_DI_H
