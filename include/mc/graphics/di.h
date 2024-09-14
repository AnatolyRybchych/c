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

MC_Error mc_di_shape_circle(MC_Di *di, MC_DiShape *shape, MC_Vec2i pos, float radius);

MC_Error mc_di_fill(MC_Di *di, MC_DiBuffer *buffer, const MC_DiShape *shape, MC_Rect2IU dst);

MC_Error mc_di_curve_dst_inverse_heatmap(MC_Di *di, MC_Size2U size,
    float heatmap[size.height][size.width], MC_Vec2f beg, size_t n, const MC_SemiBezier4f curve[n]);

MC_Error mc_di_contour_dst_inverse_heatmap(MC_Di *di, MC_Size2U size,
    float heatmap[size.height][size.width], MC_Vec2f beg, size_t n, const MC_SemiBezier4f contour[n]);

MC_Error mc_di_curve_nearest_points_map(MC_Di *di, MC_Size2U size,
    MC_Vec2f nearest[size.height][size.width], MC_Vec2f beg, size_t n, const MC_SemiBezier4f curve[n]);

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

// /// @param pos є [-1; 1]
// inline float mc_shape_get_nearest(const MC_DiShape *shape, MC_Vec2f pos){
    
// }

// /// @param pos є [-1; 1]
// inline float mc_shape_get_linear(const MC_DiShape *shape, MC_Vec2f pos){

// }

#endif // MC_DI_H
