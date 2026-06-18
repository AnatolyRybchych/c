#include <mc/graphics/di.h>

#include <mc/data/arena.h>
#include <mc/data/list.h>
#include <mc/data/bit.h>
#include <mc/util/error.h>

#include <mc/geometry/rect.h>
#include <mc/geometry/point.h>

#include <malloc.h>
#include <memory.h>
#include <stdalign.h>
#include <math.h>
#include <float.h>

typedef struct DstHeatmapStep DstHeatmapStep;
typedef struct CntHeatmapStep CntHeatmapStep;
typedef struct RectBFS RectBFS;
typedef struct Polyline Polyline;
typedef unsigned BfsStep;

struct MC_DiBuffer{
    MC_Size2U size;
    MC_AColor pixels[];
};

struct MC_DiShape{
    MC_Size2U size;
    float pixels[];
};

struct MC_Di{
    MC_Arena *arena;
};

struct RectBFSStep{
    struct RectBFSStep *next;
    MC_Vec2i cur;

    alignas(void*) uint8_t data[];
};

struct RectBFS{
    MC_Arena *arena;
    MC_Rect2IU bounds;
    size_t step_data_size;
    struct RectBFSStep *first_step;
    struct RectBFSStep *last_step;
    void *ctx;
    BfsStep (*next)(MC_Vec2i cur, void *step, void *ctx);
};

struct Polyline{
    MC_Vec2f *pts;
    size_t count;
    size_t cap;
};

typedef struct{
    MC_Vec2f a, b;
} Seg;

enum BfsStep{
    BFS_ACTION_NEXT,
    BFS_ACTION_CONTINUE,
    BFS_ACTION_BREAK,

    BFS_FLAG_VISIT = 1 << 15,
    BFS_ACTION = 0xFF,

    BFS_NEXT = BFS_ACTION_NEXT | BFS_FLAG_VISIT,
    BFS_CONTINUE = BFS_ACTION_CONTINUE | BFS_FLAG_VISIT,
    BFS_BREAK = BFS_ACTION_BREAK | BFS_FLAG_VISIT,
};

static MC_AColor blend(MC_AColor dst, MC_AColor src);
// static bool hoverover(const MC_DiBuffer *buf, MC_Vec2i pos);
static void setpx_unsafe(MC_DiBuffer *buf, MC_Vec2i pos, MC_AColor color);
// static void setpx(MC_DiBuffer *buf, MC_Vec2i pos, MC_AColor color);
static void drawpx_unsafe(MC_DiBuffer *buf, MC_Vec2i pos, MC_AColor color);
// static void drawpx(MC_DiBuffer *buf, MC_Vec2i pos, MC_AColor color);
// static MC_AColor getpx(const MC_DiBuffer *buf, MC_Vec2i pos);
static float shape_getpx(const MC_DiShape *shape, MC_Vec2i pos);
// static float shape_get_nearest(const MC_DiShape *shape, MC_Vec2f pos);
static float shape_get_linear(const MC_DiShape *shape, MC_Vec2f pos);

static MC_Error shape_circle(MC_Di *di, MC_DiShape *shape, MC_Vec2f pos, float radius);
static MC_Error shape_line(MC_Di *di, MC_DiShape *shape, MC_Vec2f p1, MC_Vec2f p2, float thikness);
static MC_Error shape_curve(MC_Di *di, MC_DiShape *shape, MC_Vec2f beg, size_t n, const MC_SemiBezier4f curve[n], float thikness);
static MC_Error shape_contour(MC_Di *di, MC_DiShape *shape, MC_Vec2f beg, size_t n, const MC_SemiBezier4f curve[n], float thikness);
static MC_Error shape_region(MC_Di *di, MC_DiShape *shape, MC_Vec2f beg, size_t n, const MC_SemiBezier4f curve[n]);
static MC_Error shape_region_contours(MC_Di *di, MC_DiShape *shape, size_t ncontours, const MC_DiContour contours[ncontours]);

static MC_Error flatten(MC_Arena *arena, MC_Vec2f beg, size_t n, const MC_SemiBezier4f curve[n], MC_Vec2f max, Polyline *out);
static void polyline_close(Polyline *pl);
static MC_Error stroke_polyline(MC_Di *di, MC_DiShape *shape, const Polyline *pl, float thikness);
static MC_Error fill_region(MC_Di *di, MC_DiShape *shape, const Polyline *loops, size_t nloops);
static MC_Error polylines_distances(MC_Di *di, MC_Size2U size, const Polyline *loops, size_t nloops, MC_DiShape **field);
static void polylines_sign_inside(MC_DiShape *field, const Polyline *loops, size_t nloops, float *crossings);
static void shape_max(MC_DiShape *dst, const MC_DiShape *src);

static MC_Error bfs_rect(MC_Arena *arena, MC_Rect2IU rect, RectBFS *bfs,
    BfsStep (*next)(MC_Vec2i cur, void *step, void *ctx), void *ctx, size_t step_data_size);
static MC_Error bfs_step(RectBFS *bfs, MC_Vec2i pos, const void *step_data);
static MC_Error bfs_run(RectBFS *bfs);

MC_Error mc_di_init(MC_Di **ret_di){
    MC_Arena *arena;
    MC_RETURN_ERROR(mc_arena_init(&arena, NULL));

    MC_Di *di;
    if(mc_alloc(NULL, sizeof(MC_Di), (void**)&di)){
        mc_arena_destroy(arena);
        return MCE_OUT_OF_MEMORY;
    }

    *di = (MC_Di){
        .arena =arena,
    };

    *ret_di = di;
    return MCE_OK;
}

void mc_di_destroy(MC_Di *di){
    mc_arena_destroy(di->arena);
    mc_free(NULL, di);
}

MC_Error mc_di_create(MC_Di *di, MC_DiBuffer **buffer, MC_Size2U size){
    (void)di;

    MC_RETURN_ERROR(mc_alloc(NULL, sizeof(MC_DiBuffer) + sizeof(MC_AColor[size.height][size.width]), (void**)buffer));
    MC_DiBuffer *res = *buffer;

    res->size = size;
    memset(res->pixels, 0, sizeof(MC_AColor[size.height][size.width]));

    *buffer = res;
    return MCE_OK;
}

void mc_di_delete(MC_Di *di, MC_DiBuffer *buffer){
    (void)di;

    if(buffer == NULL){
        return;
    }

    mc_free(NULL, buffer);
}

MC_Size2U mc_di_size(MC_DiBuffer *buffer){
    return buffer->size;
}

MC_AColor *mc_di_pixels(MC_DiBuffer *buffer){
    return buffer->pixels;
}

void mc_di_clear(MC_Di *di, MC_DiBuffer *buffer, MC_AColor color){
    (void)di;

    for(MC_AColor *px = buffer->pixels; px < buffer->pixels + buffer->size.width * buffer->size.height; px++){
        *px = color;
    }
}

void mc_di_fill_rect(MC_Di *di, MC_DiBuffer *buffer, MC_Rect2IU rect, MC_AColor color){
    (void)di;

    MC_Size2U size = buffer->size;

    int x0 = rect.x < 0 ? 0 : rect.x;
    int y0 = rect.y < 0 ? 0 : rect.y;

    int x1 = rect.x + (int)rect.width;
    int y1 = rect.y + (int)rect.height;
    if(x1 > (int)size.width){
        x1 = (int)size.width;
    }
    if(y1 > (int)size.height){
        y1 = (int)size.height;
    }

    for(int y = y0; y < y1; y++){
        MC_AColor *row = buffer->pixels + (size_t)y * size.width;
        for(int x = x0; x < x1; x++){
            row[x] = color;
        }
    }
}

MC_Error mc_di_blit(MC_Di *di, MC_DiBuffer *dst, MC_Vec2i dst_pos, MC_Vec2i src_pos, MC_Vec2i size, MC_DiBuffer *src){
    if(src == NULL){
        return MCE_INVALID_INPUT;
    }

    return mc_di_blit_pixels(di, dst, dst_pos, src_pos, size, src->size, (void*)src->pixels);
}

MC_Error mc_di_blit_pixels(MC_Di *di, MC_DiBuffer *dst, MC_Vec2i dst_pos, MC_Vec2i src_pos, MC_Vec2i size,
    MC_Size2U src_size, const MC_AColor src[src_size.height][src_size.width])
{
    (void)di;

    if(dst == NULL){
        return MCE_INVALID_INPUT;
    }

    MC_Vec2i dsrc = mc_vec2i_sub(src_pos, dst_pos);

    if(dst_pos.x < 0){
        src_pos.x -= dst_pos.x;
        size.x += dst_pos.x;
        dst_pos.x = 0;
    }

    if(dst_pos.y < 0){
        src_pos.y -= dst_pos.y;
        size.y += dst_pos.y;
        dst_pos.y = 0;
    }

    MC_Vec2i dst_end = mc_vec2i_add(dst_pos, mc_vec2i_min(mc_vec2i(dst->size.width, dst->size.height), size));
    for(int y = dst_pos.y; y < dst_end.y; y++){
        int src_y = y + dsrc.y;
        for(int x = dst_pos.x; x < dst_end.x; x++){
            int src_x = x + dsrc.x;

            (src_x < 0 || src_x >= (int)src_size.width || src_y < 0 || src_y >= (int)src_size.height)
                ? setpx_unsafe(dst, mc_vec2i(x, y), (MC_AColor){0})
                : setpx_unsafe(dst, mc_vec2i(x, y), src[src_y][src_x]);
        }
    }

    return MCE_OK;
}

MC_Error mc_di_write(MC_Di *di, MC_DiBuffer *dst, MC_Rect2IU dst_rect, MC_Rect2IU src_rect, MC_DiBuffer *src){
    if(src == NULL){
        return MCE_INVALID_INPUT;
    }

    if(src == dst){
        MC_DiBuffer *src_cp;
        MC_RETURN_ERROR(mc_arena_alloc(di->arena,
            sizeof(MC_DiBuffer) + sizeof(MC_AColor[src->size.height][src->size.width]), (void**)&src_cp));
        memcpy(src_cp, src, sizeof(MC_DiBuffer) + sizeof(MC_AColor[src->size.height][src->size.width]));

        MC_Error result = mc_di_write_pixels(di, dst, dst_rect, src_rect, src_cp->size, (void*)src_cp->pixels);
        mc_arena_reset(di->arena);
        return result;
    }

    return mc_di_write_pixels(di, dst, dst_rect, src_rect, src->size, (void*)src->pixels);
}

MC_Error mc_di_write_pixels(MC_Di *di, MC_DiBuffer *dst, MC_Rect2IU dst_rect, MC_Rect2IU src_rect,
    MC_Size2U src_size, const MC_AColor src[src_size.height][src_size.width])
{
    if(src_rect.width == dst_rect.width
    && src_rect.height == dst_rect.height)
        return mc_di_blit_pixels(di, dst,
            mc_vec2i(dst_rect.x, dst_rect.y),
            mc_vec2i(src_rect.x, src_rect.y),
            mc_vec2i(src_rect.width, src_rect.height),
            src_size, src);

    // TODO: stretch src onto dst
    return MCE_NOT_IMPLEMENTED;
}

MC_Error mc_di_fill_shape(MC_Di *di, MC_DiBuffer *buffer, const MC_DiShape *shape, MC_Rect2IU dst, MC_AColor fill_color){
    (void)di;

    MC_Vec2i dst_left_top = mc_vec2i(dst.x, dst.y);
    MC_Vec2i dst_right_bottom = mc_vec2i(dst.x + dst.width, dst.y + dst.height);

    MC_Vec2i avail_left_top = mc_vec2i_clamp(dst_left_top, mc_vec2i(0, 0), mc_vec2i(buffer->size.width, buffer->size.height));
    MC_Vec2i avail_right_bottom = mc_vec2i_clamp(dst_right_bottom, mc_vec2i(0, 0), mc_vec2i(buffer->size.width, buffer->size.height));

    float scale = dst.width / (float)shape->size.width;

    for(MC_Vec2i dst_pos = {.y = avail_left_top.y}; dst_pos.y != avail_right_bottom.y; dst_pos.y++){
        MC_Vec2f norm_pos = {.y = (dst_pos.y - dst_left_top.y) / (float)dst.height};
        for(dst_pos.x = avail_left_top.x; dst_pos.x != avail_right_bottom.x; dst_pos.x++){
            norm_pos.x = (dst_pos.x - dst_left_top.x) / (float)dst.width;

            float mag = shape_get_linear(shape, norm_pos);

            drawpx_unsafe(buffer, dst_pos, mc_acolor(mc_color(fill_color), fill_color.a * mc_clampf(mag * scale, 0, 1)));
        }
    }

    return MCE_OK;
}

MC_Error mc_di_shape_create(MC_Di *di, MC_DiShape **ret_shape, MC_Size2U size){
    (void)di;

    MC_DiShape *shape;
    MC_RETURN_ERROR(mc_alloc(NULL, sizeof(MC_DiShape) + sizeof(float[size.height][size.width]), (void**)&shape));

    shape->size = size;

    float (*pixels)[size.height][size.width] = (void*)shape->pixels;

    for(unsigned y = 0; y != size.height; y++){
        for(unsigned x = 0; x != size.width; x++){
            (*pixels)[y][x] = -1;
        }
    }

    *ret_shape = shape;
    return MCE_OK;
}

void mc_di_shape_delete(MC_Di *di, MC_DiShape *shape){
    (void)di;
    mc_free(NULL, shape);
}

MC_Error mc_di_shape_circle(MC_Di *di, MC_DiShape *shape, MC_Vec2f pos, float radius){
    MC_Error status = shape_circle(di, shape, pos, radius);
    mc_arena_reset(di->arena);
    return status;
}

MC_Error mc_di_shape_line(MC_Di *di, MC_DiShape *shape, MC_Vec2f p1, MC_Vec2f p2, float thikness){
    MC_Error status = shape_line(di, shape, p1, p2,thikness);
    mc_arena_reset(di->arena);
    return status;
}

MC_Error mc_di_shape_curve(MC_Di *di, MC_DiShape *shape, MC_Vec2f beg, size_t n, const MC_SemiBezier4f curve[n], float thikness){
    MC_Error status = shape_curve(di, shape, beg, n, curve, thikness);
    mc_arena_reset(di->arena);
    return status;
}

MC_Error mc_di_shape_contour(MC_Di *di, MC_DiShape *shape, MC_Vec2f beg, size_t n, const MC_SemiBezier4f curve[n], float thikness){
    MC_Error status = shape_contour(di, shape, beg, n, curve, thikness);
    mc_arena_reset(di->arena);
    return status;
}

MC_Error mc_di_shape_region(MC_Di *di, MC_DiShape *shape, MC_Vec2f beg, size_t n, const MC_SemiBezier4f contour[n]){
    MC_Error status = shape_region(di, shape, beg, n, contour);
    mc_arena_reset(di->arena);
    return status;
}

MC_Error mc_di_shape_region_contours(MC_Di *di, MC_DiShape *shape, size_t ncontours, const MC_DiContour contours[ncontours]){
    MC_Error status = shape_region_contours(di, shape, ncontours, contours);
    mc_arena_reset(di->arena);
    return status;
}

struct ShapeCircleCtx{
    MC_Vec2f pos;
    float radius;
    MC_DiShape *shape;
};

static BfsStep shape_circle_step(MC_Vec2i cur, void *_step, void *_ctx){
    (void)_step;

    struct ShapeCircleCtx *ctx = _ctx;

    float distance = mc_vec2f_mag(mc_vec2f_sub(mc_vec2f(ctx->pos.x, ctx->pos.y), mc_vec2f(cur.x, cur.y)));
    float (*pixels)[ctx->shape->size.height][ctx->shape->size.width] = (void*)ctx->shape->pixels;

    if((*pixels)[cur.y][cur.x] >= ctx->radius - distance){
        return BFS_CONTINUE;
    }

    (*pixels)[cur.y][cur.x] = ctx->radius - distance;
    return BFS_NEXT;
}

MC_Error shape_circle(MC_Di *di, MC_DiShape *shape, MC_Vec2f pos, float radius){
    MC_Rect2IU bounds = {
        .width = shape->size.width,
        .height = shape->size.height
    };

    pos = mc_vec2f((shape->size.width - 1) * pos.x, (shape->size.height - 1) * pos.y);

    struct ShapeCircleCtx ctx = {
        .pos = pos,
        .radius = (shape->size.width - 1) * radius,
        .shape = shape
    };

    RectBFS bfs;
    MC_RETURN_ERROR(bfs_rect(di->arena, bounds, &bfs, shape_circle_step, &ctx, 0));
    MC_RETURN_ERROR(bfs_step(&bfs, mc_vec2i(pos.x + 0.5, pos.y + 0.5), NULL));
    return bfs_run(&bfs);
}

struct ShapeLineCtx{
    MC_Vec2f p1;
    MC_Vec2f p2;
    float thikness;
    MC_DiShape *shape;
};

static BfsStep shape_line_step(MC_Vec2i cur, void *_step, void *_ctx){
    (void)_step;

    struct ShapeLineCtx *ctx = _ctx;

    MC_Vec2f curf = mc_vec2f(cur.x, cur.y);

    float distance = mc_vec2f_dst(curf,
        mc_closest_point_to_segment(ctx->p1, ctx->p2, curf));

    float (*pixels)[ctx->shape->size.height][ctx->shape->size.width] = (void*)ctx->shape->pixels;

    if((*pixels)[cur.y][cur.x] >= ctx->thikness - distance){
        return BFS_CONTINUE;
    }

    (*pixels)[cur.y][cur.x] = ctx->thikness - distance;
    return BFS_NEXT;
}

static MC_Error shape_line(MC_Di *di, MC_DiShape *shape, MC_Vec2f p1, MC_Vec2f p2, float thikness){
    MC_Rect2IU bounds = {
        .width = shape->size.width,
        .height = shape->size.height
    };

    p1 = mc_vec2f((shape->size.width - 1) * p1.x, (shape->size.height - 1) * p1.y);
    p2 = mc_vec2f((shape->size.width - 1) * p2.x, (shape->size.height - 1) * p2.y);

    struct ShapeLineCtx ctx = {
        .p1 = p1,
        .p2 = p2,
        .thikness = (shape->size.width) * thikness,
        .shape = shape
    };

    RectBFS bfs;
    MC_RETURN_ERROR(bfs_rect(di->arena, bounds, &bfs, shape_line_step, &ctx, 0));

    for(float t = 0; t <= 1; t += thikness){
        MC_Vec2f cur = mc_vec2f_lerp(p1, p2, t);
        MC_RETURN_ERROR(bfs_step(&bfs, mc_vec2i(cur.x + 0.5, cur.y + 0.5), NULL));
    }
    return bfs_run(&bfs);
}

struct ShapeCurveCtx{
    float thikness;
    MC_DiShape *shape;
};

struct ShapeCurveStep{
    MC_Vec2f nearest;
};

static BfsStep shape_curve_step(MC_Vec2i cur, void *_step, void *_ctx){
    struct ShapeCurveCtx *ctx = _ctx;
    struct ShapeCurveStep *step = _step;

    MC_Vec2f curf = mc_vec2f(cur.x, cur.y);
    float distance = mc_vec2f_dst(step->nearest, curf);

    float (*pixels)[ctx->shape->size.height][ctx->shape->size.width] = (void*)ctx->shape->pixels;

    if((*pixels)[cur.y][cur.x] >= ctx->thikness - distance){
        return BFS_ACTION_CONTINUE;
    }

    (*pixels)[cur.y][cur.x] = ctx->thikness - distance;
    return BFS_ACTION_NEXT;
}

static MC_Error flatten(MC_Arena *arena, MC_Vec2f beg, size_t n, const MC_SemiBezier4f curve[n], MC_Vec2f max, Polyline *out){
    const float inc = 0.001f;
    size_t per = (size_t)(1.0f / inc) + 2;
    size_t cap = (n + 1) * per;

    MC_Vec2f *pts;
    MC_RETURN_ERROR(mc_arena_alloc(arena, sizeof(MC_Vec2f) * cap, (void**)&pts));

    size_t count = 0;
    MC_Vec2f cur = mc_vec2f_mul(beg, max);
    for(const MC_SemiBezier4f *b = curve; b != &curve[n]; b++){
        MC_Bezier4f bez = {
            .p1 = cur,
            .c1 = mc_vec2f_mul(b->c1, max),
            .c2 = mc_vec2f_mul(b->c2, max),
            .p2 = mc_vec2f_mul(b->p2, max),
        };

        for(float t = 0; t <= 1.0f; t += inc){
            pts[count++] = mc_vec2f_clamp(mc_bezier4(bez, t), mc_vec2f(0, 0), max);
        }

        cur = bez.p2;
    }

    *out = (Polyline){.pts = pts, .count = count, .cap = cap};
    return MCE_OK;
}

static void polyline_close(Polyline *pl){
    const float inc = 0.001f;

    if(pl->count == 0){
        return;
    }

    MC_Vec2f last = pl->pts[pl->count - 1];
    MC_Vec2f first = pl->pts[0];
    if(mc_vec2f_dst(last, first) < 0.5f){
        return;
    }

    for(float t = inc; t <= 1.0f && pl->count < pl->cap; t += inc){
        pl->pts[pl->count++] = mc_vec2f_lerp(last, first, t);
    }
}

static MC_Error stroke_polyline(MC_Di *di, MC_DiShape *shape, const Polyline *pl, float thikness){
    MC_Rect2IU bounds = {
        .width = shape->size.width,
        .height = shape->size.height
    };

    struct ShapeCurveCtx ctx = {
        .thikness = shape->size.width * thikness,
        .shape = shape
    };

    RectBFS bfs;
    MC_RETURN_ERROR(bfs_rect(di->arena, bounds, &bfs, shape_curve_step, &ctx, sizeof(struct ShapeCurveStep)));

    MC_Vec2i prev = mc_vec2i(-1, -1);
    for(size_t i = 0; i < pl->count; i++){
        MC_Vec2i p = mc_vec2i(pl->pts[i].x + 0.5f, pl->pts[i].y + 0.5f);
        if(mc_vec2i_equ(prev, p)){
            continue;
        }

        prev = p;
        MC_RETURN_ERROR(bfs_step(&bfs, p, &(struct ShapeCurveStep){
            .nearest = pl->pts[i]
        }));
    }

    return bfs_run(&bfs);
}

static MC_Error polylines_distances(MC_Di *di, MC_Size2U size, const Polyline *loops, size_t nloops, MC_DiShape **field){
    MC_DiShape *f;
    MC_RETURN_ERROR(mc_arena_alloc(di->arena, sizeof(MC_DiShape) + sizeof(float[size.height][size.width]), (void**)&f));
    f->size = size;

    size_t total = 0;
    for(size_t l = 0; l < nloops; l++){
        total += loops[l].count;
    }

    Seg *seg;
    MC_RETURN_ERROR(mc_arena_alloc(di->arena, sizeof(Seg) * total, (void**)&seg));

    size_t n = 0;
    for(size_t l = 0; l < nloops; l++){
        const Polyline *pl = &loops[l];

        size_t beg = n;
        for(size_t i = 0; i < pl->count; i++){
            if(n == beg || mc_vec2f_dst(pl->pts[i], seg[n - 1].a) >= 0.7f){
                seg[n++].a = pl->pts[i];
            }
        }

        for(size_t i = beg; i < n; i++){
            seg[i].b = seg[i + 1 == n ? beg : i + 1].a;
        }
    }

    float (*pixels)[size.height][size.width] = (void*)f->pixels;
    for(unsigned y = 0; y < size.height; y++){
        for(unsigned x = 0; x < size.width; x++){
            MC_Vec2f pt = mc_vec2f(x, y);

            float best = FLT_MAX;
            for(size_t j = 0; j < n; j++){
                float d = mc_vec2f_dst(pt, mc_closest_point_to_segment(seg[j].a, seg[j].b, pt));
                if(d < best){
                    best = d;
                }
            }

            (*pixels)[y][x] = -best;
        }
    }

    *field = f;
    return MCE_OK;
}

static void polylines_sign_inside(MC_DiShape *field, const Polyline *loops, size_t nloops, float *crossings){
    float (*pixels)[field->size.height][field->size.width] = (void*)field->pixels;

    for(unsigned y = 0; y < field->size.height; y++){
        float scan = y;

        size_t m = 0;
        for(size_t l = 0; l < nloops; l++){
            const Polyline *pl = &loops[l];
            for(size_t i = 0; i < pl->count; i++){
                MC_Vec2f a = pl->pts[i];
                MC_Vec2f b = pl->pts[(i + 1) % pl->count];

                if((a.y <= scan) != (b.y <= scan)){
                    crossings[m++] = a.x + (scan - a.y) / (b.y - a.y) * (b.x - a.x);
                }
            }
        }

        for(size_t i = 1; i < m; i++){
            float v = crossings[i];
            size_t j = i;
            while(j > 0 && crossings[j - 1] > v){
                crossings[j] = crossings[j - 1];
                j--;
            }
            crossings[j] = v;
        }

        for(size_t k = 0; k + 1 < m; k += 2){
            int x0 = (int)ceilf(crossings[k]);
            int x1 = (int)floorf(crossings[k + 1]);

            x0 = x0 < 0 ? 0 : x0;
            x1 = x1 >= (int)field->size.width ? (int)field->size.width - 1 : x1;

            for(int x = x0; x <= x1; x++){
                (*pixels)[y][x] = -(*pixels)[y][x];
            }
        }
    }
}

static void shape_max(MC_DiShape *dst, const MC_DiShape *src){
    float (*d)[dst->size.height][dst->size.width] = (void*)dst->pixels;
    float (*s)[src->size.height][src->size.width] = (void*)src->pixels;

    for(unsigned y = 0; y < dst->size.height; y++){
        for(unsigned x = 0; x < dst->size.width; x++){
            if((*s)[y][x] > (*d)[y][x]){
                (*d)[y][x] = (*s)[y][x];
            }
        }
    }
}

static MC_Error fill_region(MC_Di *di, MC_DiShape *shape, const Polyline *loops, size_t nloops){
    size_t total = 0;
    for(size_t l = 0; l < nloops; l++){
        total += loops[l].count;
    }
    if(total < 3){
        return MCE_OK;
    }

    float *crossings;
    MC_RETURN_ERROR(mc_arena_alloc(di->arena, sizeof(float) * total, (void**)&crossings));

    MC_DiShape *field;
    MC_RETURN_ERROR(polylines_distances(di, shape->size, loops, nloops, &field));
    polylines_sign_inside(field, loops, nloops, crossings);
    shape_max(shape, field);

    return MCE_OK;
}

static MC_Error shape_curve(MC_Di *di, MC_DiShape *shape, MC_Vec2f beg, size_t n, const MC_SemiBezier4f curve[n], float thikness){
    MC_Vec2f max = mc_vec2f(shape->size.width - 1, shape->size.height - 1);

    Polyline pl;
    MC_RETURN_ERROR(flatten(di->arena, beg, n, curve, max, &pl));

    return stroke_polyline(di, shape, &pl, thikness);
}

static MC_Error shape_contour(MC_Di *di, MC_DiShape *shape, MC_Vec2f beg, size_t n, const MC_SemiBezier4f curve[n], float thikness){
    MC_Vec2f max = mc_vec2f(shape->size.width - 1, shape->size.height - 1);

    Polyline pl;
    MC_RETURN_ERROR(flatten(di->arena, beg, n, curve, max, &pl));
    polyline_close(&pl);

    return stroke_polyline(di, shape, &pl, thikness);
}

static MC_Error shape_region(MC_Di *di, MC_DiShape *shape, MC_Vec2f beg, size_t n, const MC_SemiBezier4f curve[n]){
    MC_Vec2f max = mc_vec2f(shape->size.width - 1, shape->size.height - 1);

    Polyline pl;
    MC_RETURN_ERROR(flatten(di->arena, beg, n, curve, max, &pl));
    polyline_close(&pl);

    return fill_region(di, shape, &pl, 1);
}

static MC_Error shape_region_contours(MC_Di *di, MC_DiShape *shape, size_t ncontours, const MC_DiContour contours[ncontours]){
    MC_Vec2f max = mc_vec2f(shape->size.width - 1, shape->size.height - 1);

    Polyline *loops;
    MC_RETURN_ERROR(mc_arena_alloc(di->arena, sizeof(Polyline) * ncontours, (void**)&loops));

    for(size_t i = 0; i < ncontours; i++){
        MC_RETURN_ERROR(flatten(di->arena, contours[i].beg, contours[i].n, contours[i].segments, max, &loops[i]));
        polyline_close(&loops[i]);
    }

    return fill_region(di, shape, loops, ncontours);
}

static MC_Error bfs_rect(MC_Arena *arena, MC_Rect2IU rect, RectBFS *bfs,
    BfsStep (*next)(MC_Vec2i cur, void *step, void *ctx), void *ctx, size_t step_data_size)
{
    *bfs = (RectBFS){
        .arena = arena,
        .bounds = rect,
        .ctx = ctx,
        .next = next,
        .step_data_size = step_data_size,
    };

    MC_RETURN_ERROR(mc_arena_alloc(arena, sizeof(struct RectBFSStep) + step_data_size, (void**)&bfs->first_step));
    bfs->first_step->next = NULL;
    bfs->last_step = bfs->first_step;

    return MCE_OK;
}

static MC_Error bfs_step(RectBFS *bfs, MC_Vec2i pos, const void *step_data){
    struct RectBFSStep *step;
    MC_RETURN_ERROR(mc_arena_alloc(bfs->arena, sizeof(struct RectBFSStep) + bfs->step_data_size, (void**)&step));

    step->cur = pos;
    step->next = NULL;
    memcpy(step->data, step_data, bfs->step_data_size);
    bfs->last_step = mc_list_add_after(bfs->last_step, step);

    return MCE_OK;
}

static MC_Error bfs_run(RectBFS *bfs){
    const MC_Rect2IU bounds = bfs->bounds;
    const MC_Vec2i max_pos = mc_vec2i(bounds.x + bounds.width, bounds.y + bounds.height);
    void *ctx = bfs->ctx;
    size_t step_data_size = bfs->step_data_size;
    MC_Arena *arena = bfs->arena;
    BfsStep (*next)(MC_Vec2i cur, void *step, void *ctx) = bfs->next;

    void *visited;
    size_t visited_size = (bounds.height * bounds.width + 7) / 8;
    MC_RETURN_ERROR(mc_arena_alloc(arena, visited_size, &visited));
    memset(visited, 0, visited_size);

    struct RectBFSStep *new_step;
    MC_RETURN_ERROR(mc_arena_alloc(arena, sizeof(struct RectBFSStep) + step_data_size, (void**)&new_step));

    struct RectBFSStep *reuse = NULL;
    struct RectBFSStep *last_step = bfs->last_step;
    struct RectBFSStep *steps = bfs->first_step->next;

    while(!mc_list_empty(steps)){
        static const MC_Vec2i offsets[4] = {
            {-1, 0},
            {1, 0},
            {0, 1},
            {0, -1}
        };

        static const MC_Vec2i *offsets_end = &offsets[sizeof offsets / sizeof *offsets];

        for(const MC_Vec2i *off = offsets; off != offsets_end; off++){
            MC_Vec2i p = mc_vec2i(steps->cur.x + off->x, steps->cur.y + off->y);
            if(p.x < bounds.x || p.x >= max_pos.x
            || p.y < bounds.y || p.y >= max_pos.y){
                continue;
            }

            size_t visited_bit = (p.y - bounds.y) * bounds.width + (p.x - bounds.x);

            if(mc_bit(visited, visited_bit)){
                continue;
            }

            memcpy(new_step, steps, sizeof(struct RectBFSStep) + step_data_size);
            new_step->cur = p;

            BfsStep step_result = next(p, new_step->data, ctx);
            BfsStep action = step_result & BFS_ACTION;
            if(action == BFS_ACTION_BREAK){
                return MCE_OK;
            }

            if(step_result & BFS_FLAG_VISIT){
                mc_bit_set(visited, visited_bit, 1);
            }

            if(action == BFS_ACTION_CONTINUE){
                continue;
            }

            last_step = mc_list_add_after(last_step, new_step);

            new_step = mc_list_empty(reuse) ? NULL : mc_list_remove(&reuse);
            if(new_step == NULL){
                MC_RETURN_ERROR(mc_arena_alloc(arena, sizeof(struct RectBFSStep) + step_data_size, (void**)&new_step));
            }
        }

        mc_list_add(&reuse, mc_list_remove(&steps));
    }

    return MCE_OK;
}

static MC_AColor blend(MC_AColor dst, MC_AColor src){
    uint8_t dfac = 255 - src.a;
    uint8_t sfac = src.a;

    return (MC_AColor){
        .a = mc_u8_clamp(255 - mc_u8_clamp(((255 - dst.a) + (255 - src.a)) / 2)),
        .r = mc_u8_clamp((dst.r * dfac + sfac * src.r) / 255),
        .g = mc_u8_clamp((dst.g * dfac + sfac * src.g) / 255),
        .b = mc_u8_clamp((dst.b * dfac + sfac * src.b) / 255)
    };
}

// static bool hoverover(const MC_DiBuffer *buf, MC_Vec2i pos){
//     return !(pos.x < 0 || pos.y < 0
//         || pos.x >= (int)buf->size.width || pos.y > (int)buf->size.height);
// }

static void setpx_unsafe(MC_DiBuffer *buf, MC_Vec2i pos, MC_AColor color){
    MC_AColor (*pixels)[buf->size.height][buf->size.width] = (void*)buf->pixels;
    (*pixels)[pos.y][pos.x] = color;
}

// static void setpx(MC_DiBuffer *buf, MC_Vec2i pos, MC_AColor color){
//     if(hoverover(buf, pos)){
//         setpx_unsafe(buf, pos, color);
//     }
// }

static void drawpx_unsafe(MC_DiBuffer *buf, MC_Vec2i pos, MC_AColor color){
    MC_AColor (*pixels)[buf->size.height][buf->size.width] = (void*)buf->pixels;
    setpx_unsafe(buf, pos, blend((*pixels)[pos.y][pos.x], color));
}

// static void drawpx(MC_DiBuffer *buf, MC_Vec2i pos, MC_AColor color){
//     if(hoverover(buf, pos)){
//         drawpx_unsafe(buf, pos, color);
//     }
// }

// static MC_AColor getpx(const MC_DiBuffer *buf, MC_Vec2i pos){
//     pos = mc_vec2i_clamp(pos, mc_vec2i(0, 0), mc_vec2i(buf->size.width - 1, buf->size.height - 1));
//     MC_AColor (*pixels)[buf->size.height][buf->size.width] = (void*)buf->pixels;
//     return (*pixels)[pos.y][pos.x];
// }

static float shape_getpx(const MC_DiShape *shape, MC_Vec2i pos){
    if(pos.x < 0 || pos.y < 0 || pos.x >= (int)shape->size.width || pos.y >= (int)shape->size.height)
        return -0.5;

    float (*pixels)[shape->size.height][shape->size.width] = (void*)shape->pixels;
    return (*pixels)[pos.y][pos.x];
}

// static float shape_get_nearest(const MC_DiShape *shape, MC_Vec2f pos){
//     pos = mc_vec2f_clamp(pos, mc_vec2f(0, 0), mc_vec2f(1, 1));
//     MC_Vec2i ipos = mc_vec2i(pos.y * (shape->size.height - 0.5), pos.x * (shape->size.width - 0.5));

//     float (*pixels)[shape->size.height][shape->size.width] = (void*)shape->pixels;
//     return (*pixels)[ipos.y][ipos.x];
// }

/// @param pos є [0; 1]
static float shape_get_linear(const MC_DiShape *shape, MC_Vec2f pos){
    MC_Vec2f absolute = mc_vec2f_mul(pos, mc_vec2f(shape->size.width - 0.5, shape->size.height - 0.5));
    MC_Vec2i iabsolute = mc_vec2i(absolute.x, absolute.y);

    MC_Vec2f displacement = mc_vec2f(absolute.x - iabsolute.x, absolute.y - iabsolute.y);

    MC_Vec2i idisplacement = mc_vec2i(
        displacement.x < 0.5 ? -1 : 1,
        displacement.y < 0.5 ? -1 : 1
    );

    displacement = mc_vec2f(
        fabsf(displacement.x - 0.5f),
        fabsf(displacement.y - 0.5f)
    );

    float px00 = shape_getpx(shape, mc_vec2i(iabsolute.x, iabsolute.y));
    float px01 = shape_getpx(shape, mc_vec2i(iabsolute.x, iabsolute.y + idisplacement.y));
    float px10 = shape_getpx(shape, mc_vec2i(iabsolute.x + idisplacement.x, iabsolute.y));
    float px11 = shape_getpx(shape, mc_vec2i(iabsolute.x + idisplacement.x, iabsolute.y + idisplacement.y));

    return mc_lerpf(
        mc_lerpf(px00, px01, displacement.y),
        mc_lerpf(px10, px11, displacement.y),
        displacement.x
    );
}