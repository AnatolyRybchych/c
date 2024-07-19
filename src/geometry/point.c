
#include <mc/geometry/point.h>

extern inline float mc_lerpf(float beg, float end, float progress);
extern inline MC_Point2F mc_lerp2f(MC_Point2F beg, MC_Point2F end, float progress);
extern inline float mc_sqrdst2f(MC_Point2F p1, MC_Point2F p2);
extern inline float mc_dst2f(MC_Point2F p1, MC_Point2F p2);
extern inline float mc_clampf(float value, float min, float max);
extern inline MC_Point2F mc_clamp2f(MC_Point2F value, MC_Point2F min, MC_Point2F max);