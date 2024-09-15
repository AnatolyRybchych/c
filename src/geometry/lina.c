#include <mc/geometry/lina.h>

float mc_lerpf(float beg, float end, float progress){
    return beg + (end - beg) * progress;
}

float mc_clampf(float val, float min, float max){
    return val < min ? min : val > max ? max : val;
}

MC_Vec2f mc_vec2f(float x, float y){
    return (MC_Vec2f){.x = x, .y = y};
}

MC_Vec2f mc_vec2f_add(MC_Vec2f lhs, MC_Vec2f rhs){
    return mc_vec2f(lhs.x + rhs.x,lhs.y + rhs.y);
}

MC_Vec2f mc_vec2f_sub(MC_Vec2f lhs, MC_Vec2f rhs){
    return mc_vec2f(lhs.x - rhs.x,lhs.y - rhs.y);
}

MC_Vec2f mc_vec2f_mul(MC_Vec2f lhs, MC_Vec2f rhs){
    return mc_vec2f(lhs.x * rhs.x,lhs.y * rhs.y);
}

MC_Vec2f mc_vec2f_div(MC_Vec2f lhs, MC_Vec2f rhs){
    return mc_vec2f(lhs.x / rhs.x,lhs.y / rhs.y);
}

MC_Vec2f mc_vec2f_lerp(MC_Vec2f beg, MC_Vec2f end, float progress){
    return mc_vec2f(
        beg.x + (end.x - beg.x) * progress,
        beg.y + (end.y - beg.y) * progress);
}

MC_Vec2f mc_vec2f_min(MC_Vec2f lhs, MC_Vec2f rhs){
    return mc_vec2f(
        lhs.x < rhs.x ? lhs.x : rhs.x,
        lhs.y < rhs.y ? lhs.y : rhs.y);
}

MC_Vec2f mc_vec2f_max(MC_Vec2f lhs, MC_Vec2f rhs){
    return mc_vec2f(
        lhs.x > rhs.x ? lhs.x : rhs.x,
        lhs.y > rhs.y ? lhs.y : rhs.y);
}

MC_Vec2f mc_vec2f_clamp(MC_Vec2f val, MC_Vec2f min, MC_Vec2f max){
    return mc_vec2f_min(max, mc_vec2f_max(min, val));
}

float mc_vec2f_sqrmag(MC_Vec2f val){
    return (val.x * val.x+ val.y * val.y);
}

float mc_vec2f_mag(MC_Vec2f val){
    return sqrt(mc_vec2f_sqrmag(val));
}

float mc_vec2f_sqrdst(MC_Vec2f vec1, MC_Vec2f vec2){
    return mc_vec2f_sqrmag(mc_vec2f_sub(vec2, vec1));
}

float mc_vec2f_dst(MC_Vec2f vec1, MC_Vec2f vec2){
    return mc_vec2f_mag(mc_vec2f_sub(vec2, vec1));
}

bool mc_vec2f_equ(MC_Vec2f lhs, MC_Vec2f rhs){
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

MC_Vec3f mc_vec3f(float x, float y, float z){
    return (MC_Vec3f){.x = x, .y = y, .z = z};
}

MC_Vec3f mc_vec3f_add(MC_Vec3f lhs, MC_Vec3f rhs){
    return mc_vec3f(lhs.x + rhs.x,lhs.y + rhs.y,lhs.z + rhs.z);
}

MC_Vec3f mc_vec3f_sub(MC_Vec3f lhs, MC_Vec3f rhs){
    return mc_vec3f(lhs.x - rhs.x,lhs.y - rhs.y,lhs.z - rhs.z);
}

MC_Vec3f mc_vec3f_mul(MC_Vec3f lhs, MC_Vec3f rhs){
    return mc_vec3f(lhs.x * rhs.x,lhs.y * rhs.y,lhs.z * rhs.z);
}

MC_Vec3f mc_vec3f_div(MC_Vec3f lhs, MC_Vec3f rhs){
    return mc_vec3f(lhs.x / rhs.x,lhs.y / rhs.y,lhs.z / rhs.z);
}

MC_Vec3f mc_vec3f_lerp(MC_Vec3f beg, MC_Vec3f end, float progress){
    return mc_vec3f(
        beg.x + (end.x - beg.x) * progress,
        beg.y + (end.y - beg.y) * progress,
        beg.z + (end.z - beg.z) * progress);
}

MC_Vec3f mc_vec3f_min(MC_Vec3f lhs, MC_Vec3f rhs){
    return mc_vec3f(
        lhs.x < rhs.x ? lhs.x : rhs.x,
        lhs.y < rhs.y ? lhs.y : rhs.y,
        lhs.z < rhs.z ? lhs.z : rhs.z);
}

MC_Vec3f mc_vec3f_max(MC_Vec3f lhs, MC_Vec3f rhs){
    return mc_vec3f(
        lhs.x > rhs.x ? lhs.x : rhs.x,
        lhs.y > rhs.y ? lhs.y : rhs.y,
        lhs.z > rhs.z ? lhs.z : rhs.z);
}

MC_Vec3f mc_vec3f_clamp(MC_Vec3f val, MC_Vec3f min, MC_Vec3f max){
    return mc_vec3f_min(max, mc_vec3f_max(min, val));
}

float mc_vec3f_sqrmag(MC_Vec3f val){
    return (val.x * val.x+ val.y * val.y+ val.z * val.z);
}

float mc_vec3f_mag(MC_Vec3f val){
    return sqrt(mc_vec3f_sqrmag(val));
}

float mc_vec3f_sqrdst(MC_Vec3f vec1, MC_Vec3f vec2){
    return mc_vec3f_sqrmag(mc_vec3f_sub(vec2, vec1));
}

float mc_vec3f_dst(MC_Vec3f vec1, MC_Vec3f vec2){
    return mc_vec3f_mag(mc_vec3f_sub(vec2, vec1));
}

bool mc_vec3f_equ(MC_Vec3f lhs, MC_Vec3f rhs){
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

MC_Vec4f mc_vec4f(float x, float y, float z, float w){
    return (MC_Vec4f){.x = x, .y = y, .z = z, .w = w};
}

MC_Vec4f mc_vec4f_add(MC_Vec4f lhs, MC_Vec4f rhs){
    return mc_vec4f(lhs.x + rhs.x,lhs.y + rhs.y,lhs.z + rhs.z,lhs.w + rhs.w);
}

MC_Vec4f mc_vec4f_sub(MC_Vec4f lhs, MC_Vec4f rhs){
    return mc_vec4f(lhs.x - rhs.x,lhs.y - rhs.y,lhs.z - rhs.z,lhs.w - rhs.w);
}

MC_Vec4f mc_vec4f_mul(MC_Vec4f lhs, MC_Vec4f rhs){
    return mc_vec4f(lhs.x * rhs.x,lhs.y * rhs.y,lhs.z * rhs.z,lhs.w * rhs.w);
}

MC_Vec4f mc_vec4f_div(MC_Vec4f lhs, MC_Vec4f rhs){
    return mc_vec4f(lhs.x / rhs.x,lhs.y / rhs.y,lhs.z / rhs.z,lhs.w / rhs.w);
}

MC_Vec4f mc_vec4f_lerp(MC_Vec4f beg, MC_Vec4f end, float progress){
    return mc_vec4f(
        beg.x + (end.x - beg.x) * progress,
        beg.y + (end.y - beg.y) * progress,
        beg.z + (end.z - beg.z) * progress,
        beg.w + (end.w - beg.w) * progress);
}

MC_Vec4f mc_vec4f_min(MC_Vec4f lhs, MC_Vec4f rhs){
    return mc_vec4f(
        lhs.x < rhs.x ? lhs.x : rhs.x,
        lhs.y < rhs.y ? lhs.y : rhs.y,
        lhs.z < rhs.z ? lhs.z : rhs.z,
        lhs.w < rhs.w ? lhs.w : rhs.w);
}

MC_Vec4f mc_vec4f_max(MC_Vec4f lhs, MC_Vec4f rhs){
    return mc_vec4f(
        lhs.x > rhs.x ? lhs.x : rhs.x,
        lhs.y > rhs.y ? lhs.y : rhs.y,
        lhs.z > rhs.z ? lhs.z : rhs.z,
        lhs.w > rhs.w ? lhs.w : rhs.w);
}

MC_Vec4f mc_vec4f_clamp(MC_Vec4f val, MC_Vec4f min, MC_Vec4f max){
    return mc_vec4f_min(max, mc_vec4f_max(min, val));
}

float mc_vec4f_sqrmag(MC_Vec4f val){
    return (val.x * val.x+ val.y * val.y+ val.z * val.z+ val.w * val.w);
}

float mc_vec4f_mag(MC_Vec4f val){
    return sqrt(mc_vec4f_sqrmag(val));
}

float mc_vec4f_sqrdst(MC_Vec4f vec1, MC_Vec4f vec2){
    return mc_vec4f_sqrmag(mc_vec4f_sub(vec2, vec1));
}

float mc_vec4f_dst(MC_Vec4f vec1, MC_Vec4f vec2){
    return mc_vec4f_mag(mc_vec4f_sub(vec2, vec1));
}

bool mc_vec4f_equ(MC_Vec4f lhs, MC_Vec4f rhs){
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
}

MC_Vec2i mc_vec2i(int x, int y){
    return (MC_Vec2i){.x = x, .y = y};
}

MC_Vec2i mc_vec2i_add(MC_Vec2i lhs, MC_Vec2i rhs){
    return mc_vec2i(lhs.x + rhs.x,lhs.y + rhs.y);
}

MC_Vec2i mc_vec2i_sub(MC_Vec2i lhs, MC_Vec2i rhs){
    return mc_vec2i(lhs.x - rhs.x,lhs.y - rhs.y);
}

MC_Vec2i mc_vec2i_mul(MC_Vec2i lhs, MC_Vec2i rhs){
    return mc_vec2i(lhs.x * rhs.x,lhs.y * rhs.y);
}

MC_Vec2i mc_vec2i_div(MC_Vec2i lhs, MC_Vec2i rhs){
    return mc_vec2i(lhs.x / rhs.x,lhs.y / rhs.y);
}

MC_Vec2i mc_vec2i_lerp(MC_Vec2i beg, MC_Vec2i end, float progress){
    return mc_vec2i(
        beg.x + (end.x - beg.x) * progress,
        beg.y + (end.y - beg.y) * progress);
}

MC_Vec2i mc_vec2i_min(MC_Vec2i lhs, MC_Vec2i rhs){
    return mc_vec2i(
        lhs.x < rhs.x ? lhs.x : rhs.x,
        lhs.y < rhs.y ? lhs.y : rhs.y);
}

MC_Vec2i mc_vec2i_max(MC_Vec2i lhs, MC_Vec2i rhs){
    return mc_vec2i(
        lhs.x > rhs.x ? lhs.x : rhs.x,
        lhs.y > rhs.y ? lhs.y : rhs.y);
}

MC_Vec2i mc_vec2i_clamp(MC_Vec2i val, MC_Vec2i min, MC_Vec2i max){
    return mc_vec2i_min(max, mc_vec2i_max(min, val));
}

float mc_vec2i_sqrmag(MC_Vec2i val){
    return (val.x * val.x+ val.y * val.y);
}

float mc_vec2i_mag(MC_Vec2i val){
    return sqrt(mc_vec2i_sqrmag(val));
}

float mc_vec2i_sqrdst(MC_Vec2i vec1, MC_Vec2i vec2){
    return mc_vec2i_sqrmag(mc_vec2i_sub(vec2, vec1));
}

float mc_vec2i_dst(MC_Vec2i vec1, MC_Vec2i vec2){
    return mc_vec2i_mag(mc_vec2i_sub(vec2, vec1));
}

bool mc_vec2i_equ(MC_Vec2i lhs, MC_Vec2i rhs){
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

MC_Vec3i mc_vec3i(int x, int y, int z){
    return (MC_Vec3i){.x = x, .y = y, .z = z};
}

MC_Vec3i mc_vec3i_add(MC_Vec3i lhs, MC_Vec3i rhs){
    return mc_vec3i(lhs.x + rhs.x,lhs.y + rhs.y,lhs.z + rhs.z);
}

MC_Vec3i mc_vec3i_sub(MC_Vec3i lhs, MC_Vec3i rhs){
    return mc_vec3i(lhs.x - rhs.x,lhs.y - rhs.y,lhs.z - rhs.z);
}

MC_Vec3i mc_vec3i_mul(MC_Vec3i lhs, MC_Vec3i rhs){
    return mc_vec3i(lhs.x * rhs.x,lhs.y * rhs.y,lhs.z * rhs.z);
}

MC_Vec3i mc_vec3i_div(MC_Vec3i lhs, MC_Vec3i rhs){
    return mc_vec3i(lhs.x / rhs.x,lhs.y / rhs.y,lhs.z / rhs.z);
}

MC_Vec3i mc_vec3i_lerp(MC_Vec3i beg, MC_Vec3i end, float progress){
    return mc_vec3i(
        beg.x + (end.x - beg.x) * progress,
        beg.y + (end.y - beg.y) * progress,
        beg.z + (end.z - beg.z) * progress);
}

MC_Vec3i mc_vec3i_min(MC_Vec3i lhs, MC_Vec3i rhs){
    return mc_vec3i(
        lhs.x < rhs.x ? lhs.x : rhs.x,
        lhs.y < rhs.y ? lhs.y : rhs.y,
        lhs.z < rhs.z ? lhs.z : rhs.z);
}

MC_Vec3i mc_vec3i_max(MC_Vec3i lhs, MC_Vec3i rhs){
    return mc_vec3i(
        lhs.x > rhs.x ? lhs.x : rhs.x,
        lhs.y > rhs.y ? lhs.y : rhs.y,
        lhs.z > rhs.z ? lhs.z : rhs.z);
}

MC_Vec3i mc_vec3i_clamp(MC_Vec3i val, MC_Vec3i min, MC_Vec3i max){
    return mc_vec3i_min(max, mc_vec3i_max(min, val));
}

float mc_vec3i_sqrmag(MC_Vec3i val){
    return (val.x * val.x+ val.y * val.y+ val.z * val.z);
}

float mc_vec3i_mag(MC_Vec3i val){
    return sqrt(mc_vec3i_sqrmag(val));
}

float mc_vec3i_sqrdst(MC_Vec3i vec1, MC_Vec3i vec2){
    return mc_vec3i_sqrmag(mc_vec3i_sub(vec2, vec1));
}

float mc_vec3i_dst(MC_Vec3i vec1, MC_Vec3i vec2){
    return mc_vec3i_mag(mc_vec3i_sub(vec2, vec1));
}

bool mc_vec3i_equ(MC_Vec3i lhs, MC_Vec3i rhs){
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

MC_Vec4i mc_vec4i(int x, int y, int z, int w){
    return (MC_Vec4i){.x = x, .y = y, .z = z, .w = w};
}

MC_Vec4i mc_vec4i_add(MC_Vec4i lhs, MC_Vec4i rhs){
    return mc_vec4i(lhs.x + rhs.x,lhs.y + rhs.y,lhs.z + rhs.z,lhs.w + rhs.w);
}

MC_Vec4i mc_vec4i_sub(MC_Vec4i lhs, MC_Vec4i rhs){
    return mc_vec4i(lhs.x - rhs.x,lhs.y - rhs.y,lhs.z - rhs.z,lhs.w - rhs.w);
}

MC_Vec4i mc_vec4i_mul(MC_Vec4i lhs, MC_Vec4i rhs){
    return mc_vec4i(lhs.x * rhs.x,lhs.y * rhs.y,lhs.z * rhs.z,lhs.w * rhs.w);
}

MC_Vec4i mc_vec4i_div(MC_Vec4i lhs, MC_Vec4i rhs){
    return mc_vec4i(lhs.x / rhs.x,lhs.y / rhs.y,lhs.z / rhs.z,lhs.w / rhs.w);
}

MC_Vec4i mc_vec4i_lerp(MC_Vec4i beg, MC_Vec4i end, float progress){
    return mc_vec4i(
        beg.x + (end.x - beg.x) * progress,
        beg.y + (end.y - beg.y) * progress,
        beg.z + (end.z - beg.z) * progress,
        beg.w + (end.w - beg.w) * progress);
}

MC_Vec4i mc_vec4i_min(MC_Vec4i lhs, MC_Vec4i rhs){
    return mc_vec4i(
        lhs.x < rhs.x ? lhs.x : rhs.x,
        lhs.y < rhs.y ? lhs.y : rhs.y,
        lhs.z < rhs.z ? lhs.z : rhs.z,
        lhs.w < rhs.w ? lhs.w : rhs.w);
}

MC_Vec4i mc_vec4i_max(MC_Vec4i lhs, MC_Vec4i rhs){
    return mc_vec4i(
        lhs.x > rhs.x ? lhs.x : rhs.x,
        lhs.y > rhs.y ? lhs.y : rhs.y,
        lhs.z > rhs.z ? lhs.z : rhs.z,
        lhs.w > rhs.w ? lhs.w : rhs.w);
}

MC_Vec4i mc_vec4i_clamp(MC_Vec4i val, MC_Vec4i min, MC_Vec4i max){
    return mc_vec4i_min(max, mc_vec4i_max(min, val));
}

float mc_vec4i_sqrmag(MC_Vec4i val){
    return (val.x * val.x+ val.y * val.y+ val.z * val.z+ val.w * val.w);
}

float mc_vec4i_mag(MC_Vec4i val){
    return sqrt(mc_vec4i_sqrmag(val));
}

float mc_vec4i_sqrdst(MC_Vec4i vec1, MC_Vec4i vec2){
    return mc_vec4i_sqrmag(mc_vec4i_sub(vec2, vec1));
}

float mc_vec4i_dst(MC_Vec4i vec1, MC_Vec4i vec2){
    return mc_vec4i_mag(mc_vec4i_sub(vec2, vec1));
}

bool mc_vec4i_equ(MC_Vec4i lhs, MC_Vec4i rhs){
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
}

MC_Vec2u mc_vec2u(unsigned x, unsigned y){
    return (MC_Vec2u){.x = x, .y = y};
}

MC_Vec2u mc_vec2u_add(MC_Vec2u lhs, MC_Vec2u rhs){
    return mc_vec2u(lhs.x + rhs.x,lhs.y + rhs.y);
}

MC_Vec2u mc_vec2u_sub(MC_Vec2u lhs, MC_Vec2u rhs){
    return mc_vec2u(lhs.x - rhs.x,lhs.y - rhs.y);
}

MC_Vec2u mc_vec2u_mul(MC_Vec2u lhs, MC_Vec2u rhs){
    return mc_vec2u(lhs.x * rhs.x,lhs.y * rhs.y);
}

MC_Vec2u mc_vec2u_div(MC_Vec2u lhs, MC_Vec2u rhs){
    return mc_vec2u(lhs.x / rhs.x,lhs.y / rhs.y);
}

MC_Vec2u mc_vec2u_lerp(MC_Vec2u beg, MC_Vec2u end, float progress){
    return mc_vec2u(
        beg.x + (end.x - beg.x) * progress,
        beg.y + (end.y - beg.y) * progress);
}

MC_Vec2u mc_vec2u_min(MC_Vec2u lhs, MC_Vec2u rhs){
    return mc_vec2u(
        lhs.x < rhs.x ? lhs.x : rhs.x,
        lhs.y < rhs.y ? lhs.y : rhs.y);
}

MC_Vec2u mc_vec2u_max(MC_Vec2u lhs, MC_Vec2u rhs){
    return mc_vec2u(
        lhs.x > rhs.x ? lhs.x : rhs.x,
        lhs.y > rhs.y ? lhs.y : rhs.y);
}

MC_Vec2u mc_vec2u_clamp(MC_Vec2u val, MC_Vec2u min, MC_Vec2u max){
    return mc_vec2u_min(max, mc_vec2u_max(min, val));
}

float mc_vec2u_sqrmag(MC_Vec2u val){
    return (val.x * val.x+ val.y * val.y);
}

float mc_vec2u_mag(MC_Vec2u val){
    return sqrt(mc_vec2u_sqrmag(val));
}

float mc_vec2u_sqrdst(MC_Vec2u vec1, MC_Vec2u vec2){
    return mc_vec2u_sqrmag(mc_vec2u_sub(vec2, vec1));
}

float mc_vec2u_dst(MC_Vec2u vec1, MC_Vec2u vec2){
    return mc_vec2u_mag(mc_vec2u_sub(vec2, vec1));
}

bool mc_vec2u_equ(MC_Vec2u lhs, MC_Vec2u rhs){
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

MC_Vec3u mc_vec3u(unsigned x, unsigned y, unsigned z){
    return (MC_Vec3u){.x = x, .y = y, .z = z};
}

MC_Vec3u mc_vec3u_add(MC_Vec3u lhs, MC_Vec3u rhs){
    return mc_vec3u(lhs.x + rhs.x,lhs.y + rhs.y,lhs.z + rhs.z);
}

MC_Vec3u mc_vec3u_sub(MC_Vec3u lhs, MC_Vec3u rhs){
    return mc_vec3u(lhs.x - rhs.x,lhs.y - rhs.y,lhs.z - rhs.z);
}

MC_Vec3u mc_vec3u_mul(MC_Vec3u lhs, MC_Vec3u rhs){
    return mc_vec3u(lhs.x * rhs.x,lhs.y * rhs.y,lhs.z * rhs.z);
}

MC_Vec3u mc_vec3u_div(MC_Vec3u lhs, MC_Vec3u rhs){
    return mc_vec3u(lhs.x / rhs.x,lhs.y / rhs.y,lhs.z / rhs.z);
}

MC_Vec3u mc_vec3u_lerp(MC_Vec3u beg, MC_Vec3u end, float progress){
    return mc_vec3u(
        beg.x + (end.x - beg.x) * progress,
        beg.y + (end.y - beg.y) * progress,
        beg.z + (end.z - beg.z) * progress);
}

MC_Vec3u mc_vec3u_min(MC_Vec3u lhs, MC_Vec3u rhs){
    return mc_vec3u(
        lhs.x < rhs.x ? lhs.x : rhs.x,
        lhs.y < rhs.y ? lhs.y : rhs.y,
        lhs.z < rhs.z ? lhs.z : rhs.z);
}

MC_Vec3u mc_vec3u_max(MC_Vec3u lhs, MC_Vec3u rhs){
    return mc_vec3u(
        lhs.x > rhs.x ? lhs.x : rhs.x,
        lhs.y > rhs.y ? lhs.y : rhs.y,
        lhs.z > rhs.z ? lhs.z : rhs.z);
}

MC_Vec3u mc_vec3u_clamp(MC_Vec3u val, MC_Vec3u min, MC_Vec3u max){
    return mc_vec3u_min(max, mc_vec3u_max(min, val));
}

float mc_vec3u_sqrmag(MC_Vec3u val){
    return (val.x * val.x+ val.y * val.y+ val.z * val.z);
}

float mc_vec3u_mag(MC_Vec3u val){
    return sqrt(mc_vec3u_sqrmag(val));
}

float mc_vec3u_sqrdst(MC_Vec3u vec1, MC_Vec3u vec2){
    return mc_vec3u_sqrmag(mc_vec3u_sub(vec2, vec1));
}

float mc_vec3u_dst(MC_Vec3u vec1, MC_Vec3u vec2){
    return mc_vec3u_mag(mc_vec3u_sub(vec2, vec1));
}

bool mc_vec3u_equ(MC_Vec3u lhs, MC_Vec3u rhs){
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

MC_Vec4u mc_vec4u(unsigned x, unsigned y, unsigned z, unsigned w){
    return (MC_Vec4u){.x = x, .y = y, .z = z, .w = w};
}

MC_Vec4u mc_vec4u_add(MC_Vec4u lhs, MC_Vec4u rhs){
    return mc_vec4u(lhs.x + rhs.x,lhs.y + rhs.y,lhs.z + rhs.z,lhs.w + rhs.w);
}

MC_Vec4u mc_vec4u_sub(MC_Vec4u lhs, MC_Vec4u rhs){
    return mc_vec4u(lhs.x - rhs.x,lhs.y - rhs.y,lhs.z - rhs.z,lhs.w - rhs.w);
}

MC_Vec4u mc_vec4u_mul(MC_Vec4u lhs, MC_Vec4u rhs){
    return mc_vec4u(lhs.x * rhs.x,lhs.y * rhs.y,lhs.z * rhs.z,lhs.w * rhs.w);
}

MC_Vec4u mc_vec4u_div(MC_Vec4u lhs, MC_Vec4u rhs){
    return mc_vec4u(lhs.x / rhs.x,lhs.y / rhs.y,lhs.z / rhs.z,lhs.w / rhs.w);
}

MC_Vec4u mc_vec4u_lerp(MC_Vec4u beg, MC_Vec4u end, float progress){
    return mc_vec4u(
        beg.x + (end.x - beg.x) * progress,
        beg.y + (end.y - beg.y) * progress,
        beg.z + (end.z - beg.z) * progress,
        beg.w + (end.w - beg.w) * progress);
}

MC_Vec4u mc_vec4u_min(MC_Vec4u lhs, MC_Vec4u rhs){
    return mc_vec4u(
        lhs.x < rhs.x ? lhs.x : rhs.x,
        lhs.y < rhs.y ? lhs.y : rhs.y,
        lhs.z < rhs.z ? lhs.z : rhs.z,
        lhs.w < rhs.w ? lhs.w : rhs.w);
}

MC_Vec4u mc_vec4u_max(MC_Vec4u lhs, MC_Vec4u rhs){
    return mc_vec4u(
        lhs.x > rhs.x ? lhs.x : rhs.x,
        lhs.y > rhs.y ? lhs.y : rhs.y,
        lhs.z > rhs.z ? lhs.z : rhs.z,
        lhs.w > rhs.w ? lhs.w : rhs.w);
}

MC_Vec4u mc_vec4u_clamp(MC_Vec4u val, MC_Vec4u min, MC_Vec4u max){
    return mc_vec4u_min(max, mc_vec4u_max(min, val));
}

float mc_vec4u_sqrmag(MC_Vec4u val){
    return (val.x * val.x+ val.y * val.y+ val.z * val.z+ val.w * val.w);
}

float mc_vec4u_mag(MC_Vec4u val){
    return sqrt(mc_vec4u_sqrmag(val));
}

float mc_vec4u_sqrdst(MC_Vec4u vec1, MC_Vec4u vec2){
    return mc_vec4u_sqrmag(mc_vec4u_sub(vec2, vec1));
}

float mc_vec4u_dst(MC_Vec4u vec1, MC_Vec4u vec2){
    return mc_vec4u_mag(mc_vec4u_sub(vec2, vec1));
}

bool mc_vec4u_equ(MC_Vec4u lhs, MC_Vec4u rhs){
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
}
