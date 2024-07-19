#ifndef MC_GEOMETRY_BEZIER_H
#define MC_GEOMETRY_BEZIER_H

#include <mc/geometry/point.h>

typedef struct MC_Bezier3F MC_Bezier3F;
typedef struct MC_Bezier4F MC_Bezier4F;
typedef struct MC_SemiBezier4F MC_SemiBezier4F;

struct MC_Bezier3F{
    MC_Point2F p1, p2;
    MC_Point2F c;
};

struct MC_Bezier4F{
    MC_Point2F p1, p2;
    MC_Point2F c1, c2;
};

struct MC_SemiBezier4F{
    MC_Point2F p2;
    MC_Point2F c1, c2;
};

inline MC_Point2F mc_bezier3(MC_Bezier3F curve, float progress){
    return mc_lerp2f(
        mc_lerp2f(curve.p1, curve.c, progress),
        mc_lerp2f(curve.c, curve.p2, progress),
        progress
    );
}

inline MC_Point2F mc_bezier4(MC_Bezier4F curve, float progress){
    return mc_bezier3(
        (MC_Bezier3F){
            .p1 = mc_lerp2f(curve.p1, curve.c1, progress),
            .c = mc_lerp2f(curve.c1, curve.c2, progress),
            .p2 = mc_lerp2f(curve.c2, curve.p2, progress)
        },
        progress
    );
}

#endif // MC_GEOMETRY_BEZIER_H
