#include <mc/util/assert.h>
#include <mc/util/error.h>
#include <mc/time.h>
#include <mc/os/file.h>

#include <mc/wm/wm.h>
#include <mc/wm/event.h>
#include <mc/wm/key.h>
#include <mc/xlib_wm/wm.h>

#include <mc/graphics/graphics.h>
#include <mc/graphics/di.h>

#include <stdbool.h>

#define SHAPE_SIZE 64

#define MARGIN 0.12f
#define SCALE (1.0f - 2.0f * MARGIN)

#define STROKE_HALF_WIDTH ((0.08f / 2.0f) * SCALE)

static const MC_AColor STROKE = {.r = 0x25, .g = 0x63, .b = 0xEB, .a = 0xFF};
static const MC_AColor PAGE   = {.r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0xFF};

static MC_Vec2f p(float x, float y){
    return mc_vec2f(MARGIN + (x / 100.0f) * SCALE, MARGIN + (y / 100.0f) * SCALE);
}

static MC_Error build_shape(MC_Di *di, MC_DiShape *shape){
    MC_SemiBezier4f seg1 = {.c1 = p(10, 0), .c2 = p(0, 50), .p2 = p(50, 50)};
    MC_RETURN_ERROR(mc_di_shape_curve(di, shape, p(50, 10), 1, &seg1, STROKE_HALF_WIDTH));

    MC_SemiBezier4f seg2 = {.c1 = p(100, 50), .c2 = p(100, 100), .p2 = p(50, 100)};
    MC_RETURN_ERROR(mc_di_shape_curve(di, shape, p(50, 50), 1, &seg2, STROKE_HALF_WIDTH));

    return MCE_OK;
}

static void draw(MC_Graphics *g, MC_Di *di, MC_DiShape *shape,
    MC_DiBuffer **buffer, unsigned *side)
{
    MC_Size2U size;
    MC_REQUIRE(mc_graphics_get_size(g, &size));

    unsigned s = (size.width < size.height ? size.width : size.height) / 2;
    if(s == 0){
        return;
    }

    if(*buffer == NULL || *side != s){
        mc_di_delete(di, *buffer);
        MC_REQUIRE(mc_di_create(di, buffer, (MC_Size2U){.width = s, .height = s}));
        *side = s;

        mc_di_clear(di, *buffer, PAGE);
        MC_REQUIRE(mc_di_fill_shape(di, *buffer, shape,
            (MC_Rect2IU){.x = 0, .y = 0, .width = s, .height = s}, STROKE));
    }

    MC_Vec2i pos = {
        .x = (int)(size.width  - s) / 2,
        .y = (int)(size.height - s) / 2,
    };

    MC_REQUIRE(mc_graphics_begin(g));
    MC_REQUIRE(mc_graphics_clear(g, mc_color(PAGE)));
    MC_REQUIRE(mc_graphics_write_pixels(g, pos,
        (MC_Size2U){.width = s, .height = s}, (void*)mc_di_pixels(*buffer), (MC_Vec2i){0, 0}));
    MC_REQUIRE(mc_graphics_end(g));
}

int main(void){
    MC_WM *wm;
    MC_REQUIRE(mc_wm_init(&wm, mc_xlib_wm_vtab));

    MC_WMWindow *window;
    MC_REQUIRE(mc_wm_window_init(wm, &window));
    MC_REQUIRE(mc_wm_window_set_title(window, MC_STRC("di svg demo - Q/Esc quit")));
    MC_REQUIRE(mc_wm_window_set_size(window, (MC_Size2U){.width = 480, .height = 480}));

    MC_Graphics *g;
    MC_REQUIRE(mc_wm_window_get_graphic(window, &g));

    MC_Di *di;
    MC_REQUIRE(mc_di_init(&di));

    MC_DiShape *shape;
    MC_REQUIRE(mc_di_shape_create(di, &shape, (MC_Size2U){.width = SHAPE_SIZE, .height = SHAPE_SIZE}));
    MC_REQUIRE(build_shape(di, shape));

    MC_DiBuffer *buffer = NULL;
    unsigned side = 0;

    bool running = true;
    bool dirty = true;
    while(running){
        MC_WMEvent event;
        while(mc_wm_poll_event(wm, &event) == MCE_OK){
            switch(event.type){
            case MC_WME_WINDOW_CLOSE_REQUESTED:
                running = false;
                break;
            case MC_WME_WINDOW_REDRAW_REQUESTED:
            case MC_WME_WINDOW_RESIZED:
                dirty = true;
                break;
            case MC_WME_KEY_DOWN:
                switch(event.as.key_down.key){
                case MC_KEY_ESCAPE:
                case MC_KEY_Q: running = false; break;
                default: break;
                }
                break;
            default:
                break;
            }
        }

        if(dirty){
            draw(g, di, shape, &buffer, &side);
            dirty = false;
        }
        mc_sleep(&(MC_Time){.nsec = 16000000});
    }

    mc_di_delete(di, buffer);
    mc_di_shape_delete(di, shape);
    mc_di_destroy(di);
    mc_wm_window_destroy(window);
    mc_wm_destroy(wm);
    return 0;
}
