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

MC_Error mc_di_create(MC_Di *di, MC_DiBuffer **buffer, MC_Size2U size){
    (void)di;

    MC_DiBuffer *res = malloc(sizeof(MC_DiBuffer) + sizeof(MC_AColor[size.height][size.width]));
    if(res == NULL){
        *buffer = NULL;
        return MCE_OUT_OF_MEMORY;
    }

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

    free(buffer);
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

    // TODO: stretch src to dst

    return MCE_OK;
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

static MC_AColor blend(MC_AColor dst, MC_AColor src){
    uint8_t dfac = 255 - dst.a;
    uint8_t sfac = src.a;

    return (MC_AColor){
        .a = mc_u8_clamp(255 - ((255 - dst.a) / 2 + (255 - src.a) / 2)),
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