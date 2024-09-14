#ifndef MC_GEOMETRY_LINA_H
#define MC_GEOMETRY_LINA_H

#include <stddef.h>
#include <math.h>
#include <stdbool.h>

#ifndef __LINA_DECL
    #define __LINA_DECL(DECL, ...) inline DECL __VA_ARGS__
#endif
#ifndef __LINA_IMPL
    #define __LINA_IMPL(DECL, ...)
#endif

#define __LINA_FUNC(...) \
    __LINA_DECL(__VA_ARGS__) \
    __LINA_IMPL(__VA_ARGS__) \

#define LINA_VEC(N, SUFIX,...) MC_Vec##N##SUFIX##__VA_ARGS__
#define LINA_VEC_LOWER(N, SUFIX, ...) mc_vec##N##SUFIX##__VA_ARGS__

#define __LINA_EACH0(CALL, ...)
#define __LINA_EACH1(CALL, ...) CALL(x, ##__VA_ARGS__)
#define __LINA_EACH2(CALL, ...) __LINA_EACH1(CALL, ##__VA_ARGS__), CALL(y, ##__VA_ARGS__)
#define __LINA_EACH3(CALL, ...) __LINA_EACH2(CALL, ##__VA_ARGS__), CALL(z, ##__VA_ARGS__)
#define __LINA_EACH4(CALL, ...) __LINA_EACH3(CALL, ##__VA_ARGS__), CALL(w, ##__VA_ARGS__)
#define LINA_EACH(N, CALL, ...) __LINA_EACH##N(CALL, ##__VA_ARGS__)

#define __LINA_EACHS0(CALL, ...)
#define __LINA_EACHS1(CALL, ...) CALL(x, ##__VA_ARGS__)
#define __LINA_EACHS2(CALL, ...) __LINA_EACHS1(CALL, ##__VA_ARGS__) CALL(y, ##__VA_ARGS__)
#define __LINA_EACHS3(CALL, ...) __LINA_EACHS2(CALL, ##__VA_ARGS__) CALL(z, ##__VA_ARGS__)
#define __LINA_EACHS4(CALL, ...) __LINA_EACHS3(CALL, ##__VA_ARGS__) CALL(w, ##__VA_ARGS__)
#define LINA_EACHS(N, CALL, ...) __LINA_EACHS##N(CALL, ##__VA_ARGS__)

#define __LINA_EACH_VEC_IMPL_SIZE(DELIM, CALL, ...) \
    CALL(2, ##__VA_ARGS__) DELIM \
    CALL(3, ##__VA_ARGS__) DELIM \
    CALL(4, ##__VA_ARGS__)

#define __LINA_GET(...) __VA_ARGS__
#define __LINA_GET2_1(SECOND, FIRST, ...) FIRST SECOND, ##__VA_ARGS__

#define __LINA_TYPEDEF_VEC(N, SUFIX) typedef struct LINA_VEC(N, SUFIX) LINA_VEC(N, SUFIX)
#define __LINA_STRUCT_VECN(N, TYPE, SUFIX) struct LINA_VEC(N, SUFIX){TYPE LINA_EACH(N, __LINA_GET);} 
#define __LINA_STRUCT_VEC(N, TYPE, SUFIX) __LINA_STRUCT_VECN(N, TYPE, SUFIX)

#define __LINA_BINOP_EVAL(X, LHS, RHS, CBINOP) (LHS.X CBINOP RHS.X)
#define __LINA_BINOP_CALL(N, TYPE, SUFIX, OP, CALL) \
__LINA_FUNC(LINA_VEC(N, SUFIX) LINA_VEC_LOWER(N, SUFIX, _##OP)(LINA_VEC(N, SUFIX) lhs, LINA_VEC(N, SUFIX) rhs), { \
    return LINA_VEC_LOWER(N, SUFIX)(LINA_EACH(N, CALL, lhs, rhs)); \
})

#define __LINA_BINOP(N, TYPE, SUFIX, OP, CBINOP) \
__LINA_FUNC(LINA_VEC(N, SUFIX) LINA_VEC_LOWER(N, SUFIX, _##OP)(LINA_VEC(N, SUFIX) lhs, LINA_VEC(N, SUFIX) rhs), { \
    return LINA_VEC_LOWER(N, SUFIX)(LINA_EACH(N, __LINA_BINOP_EVAL, lhs, rhs, CBINOP)); \
})

#define __LINA_CLAMP_OP(X, VAL, MIN, MAX) (VAL.X < MIN.X ? MIN.X : VAL.X > MAX.X ? MAX.X : VAL.X)
#define __LINA_CLAMP(N, TYPE, SUFIX) \
__LINA_FUNC(LINA_VEC(N, SUFIX) LINA_VEC_LOWER(N, SUFIX, _clamp)(LINA_VEC(N, SUFIX) val, LINA_VEC(N, SUFIX) min, LINA_VEC(N, SUFIX) max), { \
    return LINA_VEC_LOWER(N, SUFIX)(LINA_EACH(N, __LINA_CLAMP_OP, val, min, max)); \
})

#define __LINA_LERP_OP(X, BEG, END, PROGRESS) (BEG.X + (END.X - BEG.X) * PROGRESS)
#define __LINA_LERP(N, TYPE, SUFIX) \
__LINA_FUNC(LINA_VEC(N, SUFIX) LINA_VEC_LOWER(N, SUFIX, _lerp)(LINA_VEC(N, SUFIX) lhs, LINA_VEC(N, SUFIX) rhs, float progress), { \
    return LINA_VEC_LOWER(N, SUFIX)(LINA_EACH(N, __LINA_LERP_OP, lhs, rhs, progress)); \
})

#define __LINA_SQRMAG_OP(X, VAL) + (VAL.X * VAL.X)
#define __LINA_SQRMAG(N, TYPE, SUFIX) \
__LINA_FUNC(float LINA_VEC_LOWER(N, SUFIX, _sqrmag)(LINA_VEC(N, SUFIX) val), { \
    return 0 LINA_EACHS(N, __LINA_SQRMAG_OP, val); \
})

#define __LINA_MAG(N, TYPE, SUFIX) \
__LINA_FUNC(float LINA_VEC_LOWER(N, SUFIX, _mag)(LINA_VEC(N, SUFIX) val), { \
    return sqrtf(0 LINA_EACHS(N, __LINA_SQRMAG_OP, val)); \
})

#define __LINA_EQU_OP(X, LHS, RHS) && (LHS.X == RHS.X)
#define __LINA_EQU(N, TYPE, SUFIX) \
__LINA_FUNC(bool LINA_VEC_LOWER(N, SUFIX, _equ)(LINA_VEC(N, SUFIX) lhs, LINA_VEC(N, SUFIX) rhs), { \
    return true LINA_EACHS(N, __LINA_EQU_OP, lhs, rhs); \
})

#define __LINA_SQRDST(N, TYPE, SUFIX) \
__LINA_FUNC(float LINA_VEC_LOWER(N, SUFIX, _sqrdst)(LINA_VEC(N, SUFIX) lhs, LINA_VEC(N, SUFIX) rhs), { \
    return LINA_VEC_LOWER(N, SUFIX, _sqrmag)(LINA_VEC_LOWER(N, SUFIX, _sub)(rhs, lhs)); \
})

#define __LINA_DST(N, TYPE, SUFIX) \
__LINA_FUNC(float LINA_VEC_LOWER(N, SUFIX, _dst)(LINA_VEC(N, SUFIX) lhs, LINA_VEC(N, SUFIX) rhs), { \
    return sqrtf(LINA_VEC_LOWER(N, SUFIX, _sqrmag)(LINA_VEC_LOWER(N, SUFIX, _sub)(rhs, lhs))); \
})

#define __LINA_CTOR(N, TYPE, SUFIX) \
__LINA_FUNC(LINA_VEC(N, SUFIX) LINA_VEC_LOWER(N, SUFIX)(LINA_EACH(N, __LINA_GET2_1, TYPE)), { \
    return (LINA_VEC(N, SUFIX)){LINA_EACH(N, __LINA_GET)};\
})

__LINA_FUNC(float mc_lerpf(float lhs, float rhs, float progress), {
    return lhs + (rhs - lhs) * progress;
})

__LINA_FUNC(float mc_clampf(float val, float min, float max), {
    return val < min ? min : val > max ? max : val;
})

__LINA_EACH_VEC_IMPL_SIZE(;, __LINA_TYPEDEF_VEC, f);
__LINA_EACH_VEC_IMPL_SIZE(;, __LINA_TYPEDEF_VEC, lf);
__LINA_EACH_VEC_IMPL_SIZE(;, __LINA_TYPEDEF_VEC, i);
__LINA_EACH_VEC_IMPL_SIZE(;, __LINA_TYPEDEF_VEC, u);
__LINA_EACH_VEC_IMPL_SIZE(;, __LINA_TYPEDEF_VEC, zu);

__LINA_EACH_VEC_IMPL_SIZE(;, __LINA_STRUCT_VEC, float, f);
__LINA_EACH_VEC_IMPL_SIZE(;, __LINA_STRUCT_VEC, double, lf);
__LINA_EACH_VEC_IMPL_SIZE(;, __LINA_STRUCT_VEC, int, i);
__LINA_EACH_VEC_IMPL_SIZE(;, __LINA_STRUCT_VEC, unsigned, u);
__LINA_EACH_VEC_IMPL_SIZE(;, __LINA_STRUCT_VEC, size_t, zu);

__LINA_EACH_VEC_IMPL_SIZE(, __LINA_CTOR, float, f)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_BINOP, float, f, add, +)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_BINOP, float, f, sub, -)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_BINOP, float, f, mul, *)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_BINOP, float, f, div, /)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_LERP, float, f)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_CLAMP, float, f)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_SQRMAG, float, f)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_MAG, float, f)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_SQRDST, float, f)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_DST, float, f)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_EQU, float, f)

__LINA_EACH_VEC_IMPL_SIZE(, __LINA_CTOR, double, lf)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_BINOP, double, lf, add, +)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_BINOP, double, lf, sub, -)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_BINOP, double, lf, mul, *)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_BINOP, double, lf, div, /)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_LERP, double, lf)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_CLAMP, double, lf)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_SQRMAG, double, lf)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_MAG, double, lf)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_SQRDST, double, lf)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_DST, double, lf)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_EQU, double, lf)

__LINA_EACH_VEC_IMPL_SIZE(, __LINA_CTOR, int, i)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_BINOP, int, i, add, +)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_BINOP, int, i, sub, -)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_BINOP, int, i, mul, *)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_BINOP, int, i, div, /)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_LERP, int, i)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_CLAMP, int, i)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_SQRMAG, int, i)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_MAG, int, i)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_SQRDST, int, i)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_DST, int, i)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_EQU, int, i)

__LINA_EACH_VEC_IMPL_SIZE(, __LINA_CTOR, unsigned, u)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_BINOP, unsigned, u, add, +)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_BINOP, unsigned, u, sub, -)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_BINOP, unsigned, u, mul, *)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_BINOP, unsigned, u, div, /)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_LERP, unsigned, u)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_CLAMP, unsigned, u)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_SQRMAG, unsigned, u)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_MAG, unsigned, u)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_SQRDST, unsigned, u)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_DST, unsigned, u)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_EQU, unsigned, u)

__LINA_EACH_VEC_IMPL_SIZE(, __LINA_CTOR, size_t, zu)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_BINOP, size_t, zu, add, +)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_BINOP, size_t, zu, sub, -)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_BINOP, size_t, zu, mul, *)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_BINOP, size_t, zu, div, /)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_LERP, size_t, zu)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_CLAMP, size_t, zu)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_SQRMAG, size_t, zu)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_MAG, size_t, zu)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_SQRDST, size_t, zu)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_DST, size_t, zu)
__LINA_EACH_VEC_IMPL_SIZE(, __LINA_EQU, size_t, zu)

#endif // MC_GEOMETRY_LINA_H
