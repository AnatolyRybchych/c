#ifndef MC_GEOMENTRY_POINT_H
#define MC_GEOMENTRY_POINT_H

#include <math.h>

#define MC_POINT2I(X, Y) (MC_Point2I){.x = (X), .y = (Y)}
#define MC_POINT_TO2I(POINT) (MC_Point2I){.x = (POINT).x, .y = (POINT).y}

#define MC_POINT2F(X, Y) ((MC_Point2F){.x = (X), .y = (Y)})
#define MC_POINT_TO2F(POINT) MC_POINT2F((POINT).x, (POINT).y)

#define MC_POINT_EQU(P1, P2) ((P1).x == (P2).x && (P1).y == (P2).y)

typedef struct MC_Point2I MC_Point2I;
typedef struct MC_Point2F MC_Point2F;

struct MC_Point2I{
    int x, y;
};

struct MC_Point2F{
    float x, y;
};

inline float mc_lerpf(float beg, float end, float progress){
    return beg + (end - beg) * progress;
}

inline MC_Point2F mc_lerp2f(MC_Point2F beg, MC_Point2F end, float progress){
    return (MC_Point2F){
        .x = mc_lerpf(beg.x, end.x, progress),
        .y = mc_lerpf(beg.y, end.y, progress)
    };
}

inline float mc_sqrdst2f(MC_Point2F p1, MC_Point2F p2){
    MC_Point2F vec = {
        .x = p2.x - p1.x,
        .y = p2.y - p1.y
    };

    return vec.x * vec.x + vec.y * vec.y;
}

inline float mc_dst2f(MC_Point2F p1, MC_Point2F p2){
    return sqrtf(mc_sqrdst2f(p1, p2));
}

inline float mc_clampf(float value, float min, float max){
    if(value < min){
        return min;
    }

    if(value > max){
        return max;
    }

    return value;
}

inline MC_Point2F mc_clamp2f(MC_Point2F value, MC_Point2F min, MC_Point2F max){
    return (MC_Point2F){
        .x = mc_clampf(value.x, min.x, max.x),
        .y = mc_clampf(value.y, min.y, max.y)
    };
}

#endif // MC_GEOMENTRY_POINT_H
