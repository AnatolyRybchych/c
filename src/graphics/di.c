#include <mc/graphics/di.h>

#include <mc/data/arena.h>
#include <mc/data/list.h>
#include <mc/data/bit.h>

#include <malloc.h>
#include <memory.h>

typedef struct DstHeatmapStep DstHeatmapStep;
typedef struct CntHeatmapStep CntHeatmapStep;

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

static MC_Error curve_dst_inverse_heatmap(MC_Di *di, MC_Size2U size,
    float heatmap[size.height][size.width], MC_Point2F beg, size_t n, const MC_SemiBezier4F curve[n]);

static MC_Error contour_dst_inverse_heatmap(MC_Di *di, MC_Size2U size,
    float heatmap[size.height][size.width], MC_Point2F beg, size_t n, const MC_SemiBezier4F contour[n]);

static MC_Error curve_nearest_points_map(MC_Di *di, MC_Size2U size,
    MC_Point2F nearest[size.height][size.width], MC_Point2F beg, size_t n, const MC_SemiBezier4F curve[n]);

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
