#ifndef MC_GEOMETRY_BEZIER_H
#define MC_GEOMETRY_BEZIER_H

#include <mc/geometry/lina.h>

typedef struct MC_Bezier3f MC_Bezier3f;
typedef struct MC_Bezier4f MC_Bezier4f;
typedef struct MC_SemiBezier4f MC_SemiBezier4f;

struct MC_Bezier3f{
    MC_Vec2f p1, p2;
    MC_Vec2f c;
};

struct MC_Bezier4f{
    MC_Vec2f p1, p2;
    MC_Vec2f c1, c2;
};

struct MC_SemiBezier4f{
    MC_Vec2f p2;
    MC_Vec2f c1, c2;
};

inline MC_Vec2f mc_bezier3(MC_Bezier3f curve, float progress){
    return mc_vec2f_lerp(
        mc_vec2f_lerp(curve.p1, curve.c, progress),
        mc_vec2f_lerp(curve.c, curve.p2, progress),
        progress
    );
}

inline MC_Vec2f mc_bezier4(MC_Bezier4f curve, float progress){
    return mc_bezier3(
        (MC_Bezier3f){
            .p1 = mc_vec2f_lerp(curve.p1, curve.c1, progress),
            .c = mc_vec2f_lerp(curve.c1, curve.c2, progress),
            .p2 = mc_vec2f_lerp(curve.c2, curve.p2, progress)
        },
        progress
    );
}

#endif // MC_GEOMETRY_BEZIER_H
