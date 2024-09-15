#ifndef MC_GEOMETRY_LINA_H
#define MC_GEOMETRY_LINA_H

#include <math.h>
#include <stddef.h>
#include <stdbool.h>

float mc_lerpf(float beg, float end, float progress);
float mc_clampf(float val, float min, float max);
typedef struct MC_Vec2f MC_Vec2f;
typedef struct MC_Vec3f MC_Vec3f;
typedef struct MC_Vec4f MC_Vec4f;
typedef struct MC_Vec2i MC_Vec2i;
typedef struct MC_Vec3i MC_Vec3i;
typedef struct MC_Vec4i MC_Vec4i;
typedef struct MC_Vec2u MC_Vec2u;
typedef struct MC_Vec3u MC_Vec3u;
typedef struct MC_Vec4u MC_Vec4u;

MC_Vec2f mc_vec2f(float x, float y);
MC_Vec2f mc_vec2f_add(MC_Vec2f lhs, MC_Vec2f rhs);
MC_Vec2f mc_vec2f_sub(MC_Vec2f lhs, MC_Vec2f rhs);
MC_Vec2f mc_vec2f_mul(MC_Vec2f lhs, MC_Vec2f rhs);
MC_Vec2f mc_vec2f_div(MC_Vec2f lhs, MC_Vec2f rhs);
MC_Vec2f mc_vec2f_lerp(MC_Vec2f beg, MC_Vec2f end, float progress);
MC_Vec2f mc_vec2f_min(MC_Vec2f lhs, MC_Vec2f rhs);
MC_Vec2f mc_vec2f_max(MC_Vec2f lhs, MC_Vec2f rhs);
MC_Vec2f mc_vec2f_clamp(MC_Vec2f val, MC_Vec2f min, MC_Vec2f max);
float mc_vec2f_sqrmag(MC_Vec2f val);
float mc_vec2f_mag(MC_Vec2f val);
float mc_vec2f_sqrdst(MC_Vec2f vec1, MC_Vec2f vec2);
float mc_vec2f_dst(MC_Vec2f vec1, MC_Vec2f vec2);
bool mc_vec2f_equ(MC_Vec2f lhs, MC_Vec2f rhs);
MC_Vec3f mc_vec3f(float x, float y, float z);
MC_Vec3f mc_vec3f_add(MC_Vec3f lhs, MC_Vec3f rhs);
MC_Vec3f mc_vec3f_sub(MC_Vec3f lhs, MC_Vec3f rhs);
MC_Vec3f mc_vec3f_mul(MC_Vec3f lhs, MC_Vec3f rhs);
MC_Vec3f mc_vec3f_div(MC_Vec3f lhs, MC_Vec3f rhs);
MC_Vec3f mc_vec3f_lerp(MC_Vec3f beg, MC_Vec3f end, float progress);
MC_Vec3f mc_vec3f_min(MC_Vec3f lhs, MC_Vec3f rhs);
MC_Vec3f mc_vec3f_max(MC_Vec3f lhs, MC_Vec3f rhs);
MC_Vec3f mc_vec3f_clamp(MC_Vec3f val, MC_Vec3f min, MC_Vec3f max);
float mc_vec3f_sqrmag(MC_Vec3f val);
float mc_vec3f_mag(MC_Vec3f val);
float mc_vec3f_sqrdst(MC_Vec3f vec1, MC_Vec3f vec2);
float mc_vec3f_dst(MC_Vec3f vec1, MC_Vec3f vec2);
bool mc_vec3f_equ(MC_Vec3f lhs, MC_Vec3f rhs);
MC_Vec4f mc_vec4f(float x, float y, float z, float w);
MC_Vec4f mc_vec4f_add(MC_Vec4f lhs, MC_Vec4f rhs);
MC_Vec4f mc_vec4f_sub(MC_Vec4f lhs, MC_Vec4f rhs);
MC_Vec4f mc_vec4f_mul(MC_Vec4f lhs, MC_Vec4f rhs);
MC_Vec4f mc_vec4f_div(MC_Vec4f lhs, MC_Vec4f rhs);
MC_Vec4f mc_vec4f_lerp(MC_Vec4f beg, MC_Vec4f end, float progress);
MC_Vec4f mc_vec4f_min(MC_Vec4f lhs, MC_Vec4f rhs);
MC_Vec4f mc_vec4f_max(MC_Vec4f lhs, MC_Vec4f rhs);
MC_Vec4f mc_vec4f_clamp(MC_Vec4f val, MC_Vec4f min, MC_Vec4f max);
float mc_vec4f_sqrmag(MC_Vec4f val);
float mc_vec4f_mag(MC_Vec4f val);
float mc_vec4f_sqrdst(MC_Vec4f vec1, MC_Vec4f vec2);
float mc_vec4f_dst(MC_Vec4f vec1, MC_Vec4f vec2);
bool mc_vec4f_equ(MC_Vec4f lhs, MC_Vec4f rhs);
MC_Vec2i mc_vec2i(int x, int y);
MC_Vec2i mc_vec2i_add(MC_Vec2i lhs, MC_Vec2i rhs);
MC_Vec2i mc_vec2i_sub(MC_Vec2i lhs, MC_Vec2i rhs);
MC_Vec2i mc_vec2i_mul(MC_Vec2i lhs, MC_Vec2i rhs);
MC_Vec2i mc_vec2i_div(MC_Vec2i lhs, MC_Vec2i rhs);
MC_Vec2i mc_vec2i_lerp(MC_Vec2i beg, MC_Vec2i end, float progress);
MC_Vec2i mc_vec2i_min(MC_Vec2i lhs, MC_Vec2i rhs);
MC_Vec2i mc_vec2i_max(MC_Vec2i lhs, MC_Vec2i rhs);
MC_Vec2i mc_vec2i_clamp(MC_Vec2i val, MC_Vec2i min, MC_Vec2i max);
float mc_vec2i_sqrmag(MC_Vec2i val);
float mc_vec2i_mag(MC_Vec2i val);
float mc_vec2i_sqrdst(MC_Vec2i vec1, MC_Vec2i vec2);
float mc_vec2i_dst(MC_Vec2i vec1, MC_Vec2i vec2);
bool mc_vec2i_equ(MC_Vec2i lhs, MC_Vec2i rhs);
MC_Vec3i mc_vec3i(int x, int y, int z);
MC_Vec3i mc_vec3i_add(MC_Vec3i lhs, MC_Vec3i rhs);
MC_Vec3i mc_vec3i_sub(MC_Vec3i lhs, MC_Vec3i rhs);
MC_Vec3i mc_vec3i_mul(MC_Vec3i lhs, MC_Vec3i rhs);
MC_Vec3i mc_vec3i_div(MC_Vec3i lhs, MC_Vec3i rhs);
MC_Vec3i mc_vec3i_lerp(MC_Vec3i beg, MC_Vec3i end, float progress);
MC_Vec3i mc_vec3i_min(MC_Vec3i lhs, MC_Vec3i rhs);
MC_Vec3i mc_vec3i_max(MC_Vec3i lhs, MC_Vec3i rhs);
MC_Vec3i mc_vec3i_clamp(MC_Vec3i val, MC_Vec3i min, MC_Vec3i max);
float mc_vec3i_sqrmag(MC_Vec3i val);
float mc_vec3i_mag(MC_Vec3i val);
float mc_vec3i_sqrdst(MC_Vec3i vec1, MC_Vec3i vec2);
float mc_vec3i_dst(MC_Vec3i vec1, MC_Vec3i vec2);
bool mc_vec3i_equ(MC_Vec3i lhs, MC_Vec3i rhs);
MC_Vec4i mc_vec4i(int x, int y, int z, int w);
MC_Vec4i mc_vec4i_add(MC_Vec4i lhs, MC_Vec4i rhs);
MC_Vec4i mc_vec4i_sub(MC_Vec4i lhs, MC_Vec4i rhs);
MC_Vec4i mc_vec4i_mul(MC_Vec4i lhs, MC_Vec4i rhs);
MC_Vec4i mc_vec4i_div(MC_Vec4i lhs, MC_Vec4i rhs);
MC_Vec4i mc_vec4i_lerp(MC_Vec4i beg, MC_Vec4i end, float progress);
MC_Vec4i mc_vec4i_min(MC_Vec4i lhs, MC_Vec4i rhs);
MC_Vec4i mc_vec4i_max(MC_Vec4i lhs, MC_Vec4i rhs);
MC_Vec4i mc_vec4i_clamp(MC_Vec4i val, MC_Vec4i min, MC_Vec4i max);
float mc_vec4i_sqrmag(MC_Vec4i val);
float mc_vec4i_mag(MC_Vec4i val);
float mc_vec4i_sqrdst(MC_Vec4i vec1, MC_Vec4i vec2);
float mc_vec4i_dst(MC_Vec4i vec1, MC_Vec4i vec2);
bool mc_vec4i_equ(MC_Vec4i lhs, MC_Vec4i rhs);
MC_Vec2u mc_vec2u(unsigned x, unsigned y);
MC_Vec2u mc_vec2u_add(MC_Vec2u lhs, MC_Vec2u rhs);
MC_Vec2u mc_vec2u_sub(MC_Vec2u lhs, MC_Vec2u rhs);
MC_Vec2u mc_vec2u_mul(MC_Vec2u lhs, MC_Vec2u rhs);
MC_Vec2u mc_vec2u_div(MC_Vec2u lhs, MC_Vec2u rhs);
MC_Vec2u mc_vec2u_lerp(MC_Vec2u beg, MC_Vec2u end, float progress);
MC_Vec2u mc_vec2u_min(MC_Vec2u lhs, MC_Vec2u rhs);
MC_Vec2u mc_vec2u_max(MC_Vec2u lhs, MC_Vec2u rhs);
MC_Vec2u mc_vec2u_clamp(MC_Vec2u val, MC_Vec2u min, MC_Vec2u max);
float mc_vec2u_sqrmag(MC_Vec2u val);
float mc_vec2u_mag(MC_Vec2u val);
float mc_vec2u_sqrdst(MC_Vec2u vec1, MC_Vec2u vec2);
float mc_vec2u_dst(MC_Vec2u vec1, MC_Vec2u vec2);
bool mc_vec2u_equ(MC_Vec2u lhs, MC_Vec2u rhs);
MC_Vec3u mc_vec3u(unsigned x, unsigned y, unsigned z);
MC_Vec3u mc_vec3u_add(MC_Vec3u lhs, MC_Vec3u rhs);
MC_Vec3u mc_vec3u_sub(MC_Vec3u lhs, MC_Vec3u rhs);
MC_Vec3u mc_vec3u_mul(MC_Vec3u lhs, MC_Vec3u rhs);
MC_Vec3u mc_vec3u_div(MC_Vec3u lhs, MC_Vec3u rhs);
MC_Vec3u mc_vec3u_lerp(MC_Vec3u beg, MC_Vec3u end, float progress);
MC_Vec3u mc_vec3u_min(MC_Vec3u lhs, MC_Vec3u rhs);
MC_Vec3u mc_vec3u_max(MC_Vec3u lhs, MC_Vec3u rhs);
MC_Vec3u mc_vec3u_clamp(MC_Vec3u val, MC_Vec3u min, MC_Vec3u max);
float mc_vec3u_sqrmag(MC_Vec3u val);
float mc_vec3u_mag(MC_Vec3u val);
float mc_vec3u_sqrdst(MC_Vec3u vec1, MC_Vec3u vec2);
float mc_vec3u_dst(MC_Vec3u vec1, MC_Vec3u vec2);
bool mc_vec3u_equ(MC_Vec3u lhs, MC_Vec3u rhs);
MC_Vec4u mc_vec4u(unsigned x, unsigned y, unsigned z, unsigned w);
MC_Vec4u mc_vec4u_add(MC_Vec4u lhs, MC_Vec4u rhs);
MC_Vec4u mc_vec4u_sub(MC_Vec4u lhs, MC_Vec4u rhs);
MC_Vec4u mc_vec4u_mul(MC_Vec4u lhs, MC_Vec4u rhs);
MC_Vec4u mc_vec4u_div(MC_Vec4u lhs, MC_Vec4u rhs);
MC_Vec4u mc_vec4u_lerp(MC_Vec4u beg, MC_Vec4u end, float progress);
MC_Vec4u mc_vec4u_min(MC_Vec4u lhs, MC_Vec4u rhs);
MC_Vec4u mc_vec4u_max(MC_Vec4u lhs, MC_Vec4u rhs);
MC_Vec4u mc_vec4u_clamp(MC_Vec4u val, MC_Vec4u min, MC_Vec4u max);
float mc_vec4u_sqrmag(MC_Vec4u val);
float mc_vec4u_mag(MC_Vec4u val);
float mc_vec4u_sqrdst(MC_Vec4u vec1, MC_Vec4u vec2);
float mc_vec4u_dst(MC_Vec4u vec1, MC_Vec4u vec2);
bool mc_vec4u_equ(MC_Vec4u lhs, MC_Vec4u rhs);

struct MC_Vec2f{
    float x, y;
};

struct MC_Vec3f{
    float x, y, z;
};

struct MC_Vec4f{
    float x, y, z, w;
};

struct MC_Vec2i{
    int x, y;
};

struct MC_Vec3i{
    int x, y, z;
};

struct MC_Vec4i{
    int x, y, z, w;
};

struct MC_Vec2u{
    unsigned x, y;
};

struct MC_Vec3u{
    unsigned x, y, z;
};

struct MC_Vec4u{
    unsigned x, y, z, w;
};


#endif //MC_GEOMETRY_LINA_H