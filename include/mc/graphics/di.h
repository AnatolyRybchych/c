#ifndef MC_DI_H
#define MC_DI_H

#include <stdint.h>
#include <mc/graphics/graphics.h>
#include <mc/geometry/size.h>
#include <mc/geometry/bezier.h>
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

MC_Error mc_di_curve_dst_inverse_heatmap(MC_Di *di, MC_Size2U size,
    float heatmap[size.height][size.width], MC_Point2F beg, size_t n, const MC_SemiBezier4F curve[n]);

MC_Error mc_di_contour_dst_inverse_heatmap(MC_Di *di, MC_Size2U size,
    float heatmap[size.height][size.width], MC_Point2F beg, size_t n, const MC_SemiBezier4F contour[n]);

MC_Error mc_di_curve_nearest_points_map(MC_Di *di, MC_Size2U size,
    MC_Point2F nearest[size.height][size.width], MC_Point2F beg, size_t n, const MC_SemiBezier4F curve[n]);

struct MC_DiBuffer{
    MC_Size2U size;
    MC_AColor *pixels;
};

#endif // MC_DI_H
