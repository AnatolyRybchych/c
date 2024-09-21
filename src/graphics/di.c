#include <mc/graphics/di.h>

#include <mc/data/arena.h>
#include <mc/data/list.h>
#include <mc/data/bit.h>

#include <mc/geometry/rect.h>
#include <mc/geometry/point.h>

#include <malloc.h>
#include <memory.h>
#include <stdalign.h>

typedef struct DstHeatmapStep DstHeatmapStep;
typedef struct CntHeatmapStep CntHeatmapStep;
typedef struct RectBFS RectBFS;
typedef unsigned BfsStep;

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

extern inline MC_AColor mc_di_blend(MC_AColor dst, MC_AColor src);
extern inline bool mc_di_hoverover(const MC_DiBuffer *buf, MC_Vec2i pos);
extern inline void mc_di_setpx_unsafe(MC_DiBuffer *buf, MC_Vec2i pos, MC_AColor color);
extern inline void mc_di_setpx(MC_DiBuffer *buf, MC_Vec2i pos, MC_AColor color);
extern inline void mc_di_drawpx_unsafe(MC_DiBuffer *buf, MC_Vec2i pos, MC_AColor color);
extern inline void mc_di_drawpx(MC_DiBuffer *buf, MC_Vec2i pos, MC_AColor color);
extern inline MC_AColor mc_di_getpx(const MC_DiBuffer *buf, MC_Vec2i pos);
extern inline float mc_shape_getpx(const MC_DiShape *shape, MC_Vec2i pos);
extern inline float mc_shape_get_nearest(const MC_DiShape *shape, MC_Vec2f pos);
extern inline float mc_shape_get_linear(const MC_DiShape *shape, MC_Vec2f pos);

static MC_Error shape_circle(MC_Di *di, MC_DiShape *shape, MC_Vec2f pos, float radius);
static MC_Error shape_line(MC_Di *di, MC_DiShape *shape, MC_Vec2f p1, MC_Vec2f p2, float thikness);
static MC_Error shape_curve(MC_Di *di, MC_DiShape *shape, MC_Vec2f beg, size_t n, const MC_SemiBezier4f curve[n], float thikness);

static MC_Error bfs_rect(MC_Arena *arena, MC_Rect2IU rect, RectBFS *bfs,
    BfsStep (*next)(MC_Vec2i cur, void *step, void *ctx), void *ctx, size_t step_data_size);
static MC_Error bfs_step(RectBFS *bfs, MC_Vec2i pos, const void *step_data);
static MC_Error bfs_run(RectBFS *bfs);

MC_Error mc_di_init(MC_Di **ret_di){
    MC_Arena *arena;
    MC_RETURN_ERROR(mc_arena_init(&arena));

    MC_Di *di = malloc(sizeof(MC_Di));
    if(di == NULL){
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
    free(di);
}

void mc_di_clear(MC_Di *di, MC_DiBuffer *buffer, MC_AColor color){
    (void)di;

    for(MC_AColor *px = buffer->pixels; px < buffer->pixels + buffer->size.width * buffer->size.height; px++){
        *px = color;
    }
}

MC_Error mc_di_shape_create(MC_Di *di, MC_DiShape **ret_shape, MC_Size2U size){
    (void)di;

    MC_DiShape *shape = malloc(sizeof(MC_DiShape) + sizeof(float[size.height][size.width]));
    if(shape == NULL){
        return MCE_OUT_OF_MEMORY;
    }

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
    free(shape);
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

MC_Error mc_di_fill(MC_Di *di, MC_DiBuffer *buffer, const MC_DiShape *shape, MC_Rect2IU dst, MC_AColor fill_color){
    (void)di;

    MC_Vec2i dst_left_top = mc_vec2i(dst.x, dst.y);
    MC_Vec2i dst_right_bottom = mc_vec2i(dst.x + dst.width, dst.y + dst.height);

    MC_Vec2i avail_left_top = mc_vec2i_clamp(dst_left_top, mc_vec2i(0, 0), mc_vec2i(buffer->size.width, buffer->size.height));
    MC_Vec2i avail_right_bottom = mc_vec2i_clamp(dst_right_bottom, mc_vec2i(0, 0), mc_vec2i(buffer->size.width, buffer->size.height));

    float scale = dst.width / (float)shape->size.width;

    // MC_Vec2f shape_size = mc_vec2f(shape->size.width, shape->size.height);

    for(MC_Vec2i dst_pos = {.y = avail_left_top.y}; dst_pos.y != avail_right_bottom.y; dst_pos.y++){
        MC_Vec2f norm_pos = {.y = (dst_pos.y - dst_left_top.y) / (float)dst.height};
        for(dst_pos.x = avail_left_top.x; dst_pos.x != avail_right_bottom.x; dst_pos.x++){
            norm_pos.x = (dst_pos.x - dst_left_top.x) / (float)dst.width;

            float mag = mc_shape_get_linear(shape, norm_pos);

            mc_di_drawpx_unsafe(buffer, dst_pos, mc_acolor(mc_color(fill_color), fill_color.a * mc_clampf(mag * scale, 0, 1)));
        }
    }

    return MCE_OK;
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

static MC_Error shape_curve(MC_Di *di, MC_DiShape *shape, MC_Vec2f beg, size_t n, const MC_SemiBezier4f curve[n], float thikness){
    MC_Rect2IU bounds = {
        .width = shape->size.width,
        .height = shape->size.height
    };

    MC_Vec2f shape_max_pos = mc_vec2f(shape->size.width - 1, shape->size.height - 1);
    beg = mc_vec2f_mul(beg, shape_max_pos);

    struct ShapeCurveCtx ctx = {
        .thikness = shape->size.width * thikness,
        .shape = shape
    };

    RectBFS bfs;
    MC_RETURN_ERROR(bfs_rect(di->arena, bounds, &bfs, shape_curve_step, &ctx, sizeof(struct ShapeCurveStep)));

    MC_Vec2i prev = mc_vec2i(beg.x + 0.5, beg.y + 0.5);
    for(const MC_SemiBezier4f *b = curve; b != &curve[n]; b++){
        for(float progress = 0, inc = 0.001; progress <= 1; progress += inc){
            MC_Vec2f on_curve = mc_bezier4((MC_Bezier4f){
                .p1 = beg,
                .c1 =  mc_vec2f_mul(b->c1, shape_max_pos),
                .c2 =  mc_vec2f_mul(b->c2, shape_max_pos),
                .p2 =  mc_vec2f_mul(b->p2, shape_max_pos)
            }, progress);

            on_curve = mc_vec2f_clamp(on_curve, mc_vec2f(0, 0), shape_max_pos);

            MC_Vec2i p = mc_vec2i(on_curve.x + 0.5, on_curve.y + 0.5);
            if(mc_vec2i_equ(prev, p)){
                continue;
            }

            MC_RETURN_ERROR(bfs_step(&bfs, p, &(struct ShapeCurveStep){
                .nearest = on_curve
            }));
        }

        beg = b->p2;
    }

    return bfs_run(&bfs);
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
