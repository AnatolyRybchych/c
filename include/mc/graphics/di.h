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

MC_Error mc_di_create(MC_Di *di, MC_DiBuffer **buffer, MC_Size2U size);
void mc_di_delete(MC_Di *di, MC_DiBuffer *buffer);

MC_Size2U mc_di_size(MC_DiBuffer *buffer);
MC_AColor *mc_di_pixels(MC_DiBuffer *buffer);

void mc_di_clear(MC_Di *di, MC_DiBuffer *buffer, MC_AColor color);
MC_Error mc_di_fill_shape(MC_Di *di, MC_DiBuffer *buffer, const MC_DiShape *shape, MC_Rect2IU dst, MC_AColor fill_color);

MC_Error mc_di_shape_create(MC_Di *di, MC_DiShape **shape, MC_Size2U size);
void mc_di_shape_delete(MC_Di *di, MC_DiShape *shape);

MC_Error mc_di_shape_circle(MC_Di *di, MC_DiShape *shape, MC_Vec2f pos, float radius);
MC_Error mc_di_shape_line(MC_Di *di, MC_DiShape *shape, MC_Vec2f p1, MC_Vec2f p2, float thikness);
MC_Error mc_di_shape_curve(MC_Di *di, MC_DiShape *shape, MC_Vec2f beg, size_t n, const MC_SemiBezier4f curve[n], float thikness);


#endif // MC_DI_H
