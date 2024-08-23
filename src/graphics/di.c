#include <mc/graphics/di.h>

#include <mc/data/arena.h>
#include <mc/data/list.h>
#include <mc/data/bit.h>

#include <mc/geometry/rect.h>

#include <malloc.h>
#include <memory.h>
#include <stdalign.h>

typedef struct DstHeatmapStep DstHeatmapStep;
typedef struct CntHeatmapStep CntHeatmapStep;
typedef struct RectBFS RectBFS;

typedef unsigned BfsStep;
enum BfsStep{
    BFS_NEXT,
    BFS_CONTINUE,
    BFS_BREAK,
};

struct DstHeatmapStep{
    struct DstHeatmapStep *next;
    MC_Point2F src;
    MC_Point2I dst;
};

struct CntHeatmapStep{
    struct CntHeatmapStep *next;
    size_t count;
    MC_Point2I cur;
};

struct MC_Di{
    MC_Blend blend;

    MC_Arena *arena;
};

struct MC_DiShape{
    MC_Size2U size;
    float pixels[];
};

struct RectBFSStep{
    struct RectBFSStep *next;
    MC_Point2I cur;

    alignas(void*) uint8_t data[];
};

struct RectBFS{
    MC_Arena *arena;
    MC_Rect2IU bounds;
    size_t step_data_size;
    struct RectBFSStep *first_step;
    struct RectBFSStep *last_step;
    void *ctx;
    BfsStep (*next)(MC_Point2I cur, void *step, void *ctx);
};

static MC_Error curve_dst_inverse_heatmap(MC_Di *di, MC_Size2U size,
    float heatmap[size.height][size.width], MC_Point2F beg, size_t n, const MC_SemiBezier4F curve[n]);

static MC_Error contour_dst_inverse_heatmap(MC_Di *di, MC_Size2U size,
    float heatmap[size.height][size.width], MC_Point2F beg, size_t n, const MC_SemiBezier4F contour[n]);

static MC_Error curve_nearest_points_map(MC_Di *di, MC_Size2U size,
    MC_Point2F nearest[size.height][size.width], MC_Point2F beg, size_t n, const MC_SemiBezier4F curve[n]);

MC_Error shape_circle(MC_Di *di, MC_DiShape *shape, MC_Point2I pos, float radius);

static MC_Error bfs_rect(MC_Arena *arena, MC_Rect2IU rect, RectBFS *bfs,
    BfsStep (*next)(MC_Point2I cur, void *step, void *ctx), void *ctx, size_t step_data_size);

static MC_Error bfs_step(RectBFS *bfs, MC_Point2I pos, const void *step_data);

static MC_Error bfs_run(RectBFS *bfs);

MC_Error mc_di_init(MC_Di **ret_di){
    MC_Arena *arena;
    MC_RETURN_ERROR(mc_arena_init(&arena));

    MC_Di *di = malloc(sizeof(MC_Blend));
    if(di == NULL){
        mc_arena_destroy(arena);
        return MCE_OUT_OF_MEMORY;
    }


    *di = (MC_Di){
        .arena =arena,
        .blend = MC_BLEND_SET_SRC
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
            (*pixels)[y][x] = -INFINITY;
        }
    }

    *ret_shape = shape;
    return MCE_OK;
}

void mc_di_shape_delete(MC_Di *di, MC_DiShape *shape){
    (void)di;
    free(shape);
}

MC_Error mc_di_shape_circle(MC_Di *di, MC_DiShape *shape, MC_Point2I pos, float radius){
    MC_Error status = shape_circle(di, shape, pos, radius);
    mc_arena_reset(di->arena);
    return status;
}

MC_Error mc_di_fill(MC_Di *di, MC_DiBuffer *buffer, const MC_DiShape *shape){
    (void)di;

    MC_AColor (*pixels)[buffer->size.height][buffer->size.width] = (void*)buffer->pixels;
    float (*shape_pixels)[shape->size.height][shape->size.width] = (void*)shape->pixels;

    for(unsigned y = 0; y != shape->size.height; y++){
        for(unsigned x = 0; x != shape->size.width; x++){
            (*pixels)[y][x] = (MC_AColor){
                .a = 255,
                .r = mc_clampf((*shape_pixels)[y][x], 0, 1) * 255
            };
        }
    }

    return MCE_OK;
}

MC_Error mc_di_curve_dst_inverse_heatmap(MC_Di *di, MC_Size2U size,
    float heatmap[size.height][size.width], MC_Point2F beg, size_t n, const MC_SemiBezier4F curve[n])
{
    MC_Error status = curve_dst_inverse_heatmap(di, size, heatmap, beg, n, curve);
    mc_arena_reset(di->arena);
    return status;
}

MC_Error mc_di_contour_dst_inverse_heatmap(MC_Di *di, MC_Size2U size,
    float heatmap[size.height][size.width], MC_Point2F beg, size_t n, const MC_SemiBezier4F contour[n])
{
    MC_Error status = contour_dst_inverse_heatmap(di, size, heatmap, beg, n, contour);
    mc_arena_reset(di->arena);
    return status;
}

MC_Error mc_di_curve_nearest_points_map(MC_Di *di, MC_Size2U size,
    MC_Point2F nearest[size.height][size.width], MC_Point2F beg, size_t n, const MC_SemiBezier4F curve[n])
{
    MC_Error status = curve_nearest_points_map(di, size, nearest, beg, n, curve);
    mc_arena_reset(di->arena);
    return status;
}

static MC_Error curve_dst_inverse_heatmap(MC_Di *di, MC_Size2U size,
    float heatmap[size.height][size.width], MC_Point2F beg, size_t n, const MC_SemiBezier4F curve[n])
{
    MC_Point2F (*nearest)[size.height][size.width];
    MC_RETURN_ERROR(mc_arena_alloc(di->arena, sizeof(MC_Point2F[size.height][size.width]), (void*)&nearest));
    MC_RETURN_ERROR(curve_nearest_points_map(di, size, *nearest, beg, n, curve));

    for(size_t y = 0; y < size.height; y++){
        for(size_t x = 0; x < size.width; x++){
            heatmap[y][x] = mc_sqrdst2f((*nearest)[y][x], MC_POINT2F(x, y));
        }
    }

    return MCE_OK;
}

static MC_Error contour_dst_inverse_heatmap(MC_Di *di, MC_Size2U size,
    float heatmap[size.height][size.width], MC_Point2F beg, size_t n, const MC_SemiBezier4F contour[n])
{
    size_t contour_size = n;
    const MC_SemiBezier4F *full_contour = contour;

    if(n && !MC_POINT_EQU(beg, contour[n-1].p2)){
        contour_size = n + 1;
        MC_SemiBezier4F *c;
        MC_RETURN_ERROR(mc_arena_alloc(di->arena, sizeof(MC_SemiBezier4F[contour_size]), (void*)&c));
        memcpy(c, contour, sizeof(MC_SemiBezier4F[n]));

        c[n] = (MC_SemiBezier4F){
            .c1 = contour[n-1].p2,
            .c2 = beg,
            .p2 = beg
        };

        full_contour = c;
    }

    return curve_dst_inverse_heatmap(di, size, heatmap, beg, contour_size, full_contour);
}

static MC_Error curve_nearest_points_map(MC_Di *di, MC_Size2U size,
    MC_Point2F nearest[size.height][size.width], MC_Point2F beg, size_t n, const MC_SemiBezier4F curve[n])
{
    //TODO: consider using array based queue
    DstHeatmapStep first_step = {.next = NULL};
    DstHeatmapStep *last_step = &first_step;
    DstHeatmapStep *reuse = NULL;

    MC_Point2I prev = MC_POINT_TO2I(beg);

    for(const MC_SemiBezier4F *b = curve; b != &curve[n]; b++){
        for(float progress = 0, inc = 0.001; progress <= 1; progress += inc){
            MC_Point2F on_curve = mc_bezier4((MC_Bezier4F){
                .p1 = beg,
                .c1 = b->c1,
                .c2 = b->c2,
                .p2 = b->p2
            }, progress);

            on_curve = mc_clamp2f(on_curve, MC_POINT2F(0, 0), MC_POINT2F(size.width, size.height));
            MC_Point2I p = MC_POINT_TO2I(on_curve);

            if(MC_POINT_EQU(prev, p)){
                continue;
            }

            //TODO: repeat for all pixels in between prev and p
            DstHeatmapStep *new;
            MC_RETURN_ERROR(mc_arena_alloc(di->arena, sizeof(DstHeatmapStep), (void**)&new));
            *new = (DstHeatmapStep){
                .src = on_curve,
                .dst = p,
            };

            last_step = mc_list_add_after(last_step, new);

            prev = p;
        }

        beg = b->p2;
    }

    DstHeatmapStep *steps = first_step.next;
    while(!mc_list_empty(steps)){
        static const MC_Point2I offsets[4] = {
            {-1, 0},
            {1, 0},
            {0, -1},
            {0, 1}
        };

        for(const MC_Point2I *off = offsets; off != &offsets[sizeof offsets / sizeof *offsets]; off++){
            MC_Point2I p = MC_POINT2I(steps->dst.x + off->x, steps->dst.y + off->y);
            if(p.x < 0 || p.x >= (int)size.width
            || p.y < 0 || p.y >= (int)size.height){
                continue;
            }

            float dst = mc_sqrdst2f(steps->src, MC_POINT_TO2F(p));
            float cur_dst = mc_sqrdst2f(nearest[p.y][p.x], MC_POINT_TO2F(p));
            if(cur_dst <= dst){
                continue;
            }

            nearest[p.y][p.x] = steps->src;

            DstHeatmapStep *new = mc_list_empty(reuse) ? NULL : mc_list_remove(&reuse);
            if(new == NULL){
                MC_RETURN_ERROR(mc_arena_alloc(di->arena, sizeof(DstHeatmapStep), (void**)&new));
            }

            *new = (DstHeatmapStep){
                .src = steps->src,
                .dst = p,
            };

            last_step = mc_list_add_after(last_step, new);
        }

        mc_list_add(&reuse, mc_list_remove(&steps));
    }

    return MCE_OK;
}

struct ShapeCircleCtx{
    MC_Point2I pos;
    float radius;
    MC_DiShape *shape;
};

static BfsStep shape_circle_step(MC_Point2I cur, void *_step, void *_ctx){
    (void)_step;

    struct ShapeCircleCtx *ctx = _ctx;

    float distance = mc_dst2f(MC_POINT_TO2F(cur), MC_POINT_TO2F(ctx->pos));
    float (*pixels)[ctx->shape->size.height][ctx->shape->size.width] = (void*)ctx->shape->pixels;

    if((*pixels)[cur.y][cur.x] >= ctx->radius - distance){
        return BFS_CONTINUE;
    }

    (*pixels)[cur.y][cur.x] = ctx->radius - distance;
    return BFS_NEXT;
}

MC_Error shape_circle(MC_Di *di, MC_DiShape *shape, MC_Point2I pos, float radius){
    MC_Rect2IU bounds = {
        .width = shape->size.width,
        .height = shape->size.height
    };

    struct ShapeCircleCtx ctx = {
        .pos = pos,
        .radius = radius,
        .shape = shape
    };

    RectBFS bfs;
    MC_RETURN_ERROR(bfs_rect(di->arena, bounds, &bfs, shape_circle_step, &ctx, 0));
    MC_RETURN_ERROR(bfs_step(&bfs, pos, NULL));
    return bfs_run(&bfs);
}

static MC_Error bfs_rect(MC_Arena *arena, MC_Rect2IU rect, RectBFS *bfs,
    BfsStep (*next)(MC_Point2I cur, void *step, void *ctx), void *ctx, size_t step_data_size)
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

static MC_Error bfs_step(RectBFS *bfs, MC_Point2I pos, const void *step_data){
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
    const MC_Point2I max_pos = MC_POINT2I(bounds.x + bounds.width, bounds.y + bounds.height);
    void *ctx = bfs->ctx;
    size_t step_data_size = bfs->step_data_size;
    MC_Arena *arena = bfs->arena;
    BfsStep (*next)(MC_Point2I cur, void *step, void *ctx) = bfs->next;

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
        static const MC_Point2I offsets[4] = {
            {-1, 0},
            {1, 0},
            {0, 1},
            {0, -1}
        };

        static const MC_Point2I *offsets_end = &offsets[sizeof offsets / sizeof *offsets];

        for(const MC_Point2I *off = offsets; off != offsets_end; off++){
            MC_Point2I p = MC_POINT2I(steps->cur.x + off->x, steps->cur.y + off->y);
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
            BfsStep action = next(p, new_step, ctx);
            if(action == BFS_BREAK){
                return MCE_OK;
            }

            mc_bit_set(visited, visited_bit, 1);

            if(action == BFS_CONTINUE){
                continue;
            }

            last_step = mc_list_add_after(last_step, new_step);

            new_step = mc_list_empty(reuse) ? NULL : mc_list_remove(&reuse);
            if(new_step == NULL){
                MC_RETURN_ERROR(mc_arena_alloc(arena, sizeof(struct RectBFSStep), (void**)&new_step));
            }
        }

        mc_list_add(&reuse, mc_list_remove(&steps));
    }

    return MCE_OK;
}
