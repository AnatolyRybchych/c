#ifndef MC_GEOMETRY_POINT_H
#define MC_GEOMETRY_POINT_H

#include <mc/geometry/lina.h>

inline float mc_line_to_point_dst(MC_Vec2f l1, MC_Vec2f l2, MC_Vec2f p){
    return fabs((l2.y - l1.y) * p.x - (l2.x - l1.x) * p.y + l2.x * l1.y - l2.y * l1.x)
        / sqrt(pow(l2.y - l1.y, 2) + pow(l2.x - l1.x, 2));
}

inline MC_Vec2f mc_closest_point_to_segment(MC_Vec2f l1, MC_Vec2f l2, MC_Vec2f p){
    double segment_sqrlen = mc_vec2f_sqrdst(l1, l2);
    
    if (segment_sqrlen == 0.0) {
        return l1;
    }

    double t = ((p.x - l1.x) * (l2.x - l1.x) + (p.y - l1.y) * (l2.y - l1.y)) / segment_sqrlen;

    return mc_vec2f_lerp(l1, l2, mc_clampf(t, 0, 1.0));
}

#endif // MC_GEOMETRY_POINT_H
