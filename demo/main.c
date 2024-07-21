#include <stdio.h>

#include <mc/dlib.h>
#include <mc/sched.h>
#include <mc/time.h>
#include <mc/data/pqueue.h>
#include <mc/data/list.h>
#include <mc/data/string.h>
#include <mc/data/struct.h>
#include <mc/io/stream.h>
#include <mc/io/file.h>
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

    float (*heatmap)[buf.size.height][buf.size.width] = malloc(sizeof(*heatmap));
    mc_di_contour_dst_inverse_heatmap(di, buf.size, *heatmap, MC_POINT2F(100, 100), 1, &(MC_SemiBezier4F){
        .c1 = {200, 100},
        .c2 = {100, 200},
        .p2 = {200, 200}
    });

    MC_AColor (*pixels)[buf.size.height][buf.size.width] = (void*)buf.pixels;

    mc_di_clear(di, &buf, (MC_AColor){.r = 10, .g = 4, .b = 2});
    for(size_t y = 0; y < buf.size.height; y++){
        for(size_t x = 0; x < buf.size.width; x++){
            (*pixels)[y][x] = (MC_AColor){
                .r =  (2.0 - mc_clampf((*heatmap)[y][x], 0, 2)) * 125.5
            };
        }
    }


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
            MC_REQUIRE(mc_graphics_write_pixels(g, (MC_Point2I){.x = 0, .y = 0}, buf.size, (void*)buf.pixels, (MC_Point2I){0, 0}));
            MC_REQUIRE(mc_graphics_end(g));
        }break;
        case MC_WME_KEY_DOWN:
            mc_fmt(MC_STDOUT, "KEY_DOWN %s\n", mc_key_str(event.as.key_down.key));
            break;
        case MC_WME_KEY_UP:
            mc_fmt(MC_STDOUT, "KEY_UP %s\n", mc_key_str(event.as.key_up.key));
            break;
        default:
            mc_fmt(MC_STDOUT, "event %s\n", mc_wm_event_type_str(event.type));
            break;
        }
    }

    mc_wm_destroy(wm);
}

