#include <stdio.h>

#include <mc/dlib.h>
#include <mc/sched.h>
#include <mc/time.h>
#include <mc/data/pqueue.h>
#include <mc/data/list.h>
#include <mc/data/string.h>
#include <mc/data/struct.h>
#include <mc/io/stream.h>
#include <mc/os/file.h>
#include <mc/util/assert.h>

#include <mc/wm/wm.h>
#include <mc/wm/event.h>
#include <mc/xlib_wm/wm.h>

#include <mc/graphics/graphics.h>
#include <mc/graphics/di.h>
#include <mc/xlib_wm/graphics.h>

#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

int main(){
    MC_Di *di;
    MC_REQUIRE(mc_di_init(&di));

    MC_AColor buf_pixels[800*600] = {0};
    MC_DiBuffer buf = {
        .size = {
            .width = 800,
            .height = 600,
        },
        .pixels = buf_pixels
    };

    MC_DiShape *shape;
    mc_di_shape_create(di, &shape, (MC_Size2U){.width = 50, .height = 50});
    mc_di_shape_line(di, shape, mc_vec2f(0.1, 0.1), mc_vec2f(0.9, 0.9), 0.03);
    mc_di_shape_line(di, shape, mc_vec2f(0.1, 0.9), mc_vec2f(0.9, 0.1), 0.03);

    mc_di_shape_curve(di, shape, mc_vec2f(0.1, 0.1), 1, &(MC_SemiBezier4f){
        .p2 = mc_vec2f(0.9, 0.9),
        .c1 = mc_vec2f(0.1, 0.9),
        .c2 = mc_vec2f(0.9, 0.1),
    }, 0.03);

    mc_di_shape_curve(di, shape, mc_vec2f(0.9, 0.1), 1, &(MC_SemiBezier4f){
        .p2 = mc_vec2f(0.1, 0.9),
        .c1 = mc_vec2f(0.9, 0.9),
        .c2 = mc_vec2f(0.1, 0.1),
    }, 0.03);
    // mc_di_shape_circle(di, shape, mc_vec2f(0.5, 0.5), 0.2);

    // for(int y = 0; y < (int)shape->size.height; y++){
    //     for(int x = 0; x < (int)shape->size.width; x++){
    //         printf("%f\t", mc_shape_getpx(shape, mc_vec2i(x, y)));
    //     }
    //     printf("\n");
    // }

    mc_di_clear(di, &buf, (MC_AColor){.a = 255, .r = 60, .g = 60, .b = 60});

    mc_di_fill(di, &buf, shape, MC_RECT2IU(100, 100, 300, 300), (MC_AColor){
        .a = 255,
        .r = 120,
        .g = 60,
        .b = 30,
    });
    mc_di_shape_delete(di, shape);

    MC_WM *wm;
    MC_REQUIRE(mc_wm_init(&wm, mc_xlib_wm_vtab));

    MC_WMWindow *window;
    MC_REQUIRE(mc_wm_window_init(wm, &window));

    MC_Graphics *g;
    MC_REQUIRE(mc_wm_window_get_graphic(window, &g));

    while(true){
        MC_WMEvent event;
        if(MC_REQUIRE(mc_wm_poll_event(wm, &event)) != MCE_OK){
            mc_sleep(&(MC_Time){.nsec = 1000000});
            continue;
        }

        switch (event.type){
        case MC_WME_WINDOW_REDRAW_REQUESTED:{
            MC_REQUIRE(mc_graphics_begin(g));
            MC_REQUIRE(mc_graphics_write_pixels(g, (MC_Vec2i){.x = 0, .y = 0}, buf.size, (void*)buf.pixels, (MC_Vec2i){0, 0}));
            MC_REQUIRE(mc_graphics_end(g));
        }break;
        case MC_WME_KEY_DOWN:
            // mc_fmt(MC_STDOUT, "KEY_DOWN %s\n", mc_key_str(event.as.key_down.key));
            break;
        case MC_WME_KEY_UP:
            // mc_fmt(MC_STDOUT, "KEY_UP %s\n", mc_key_str(event.as.key_up.key));
            break;
        default:
            // mc_fmt(MC_STDOUT, "event %s\n", mc_wm_event_type_str(event.type));
            break;
        }
    }

    mc_wm_destroy(wm);
}

