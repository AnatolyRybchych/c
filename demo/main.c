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
#include <mc/xlib_wm/graphics.h>

#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

int main(){
    MC_WM *wm;
    MC_REQUIRE(mc_wm_init(&wm, mc_xlib_wm_vtab));

    MC_WMWindow *window;
    MC_REQUIRE(mc_wm_window_init(wm, &window));

    MC_Graphics *g;
    MC_REQUIRE(mc_wm_window_get_graphic(window, &g));

    MC_AColor pixels[16][16];
    for(size_t y = 0; y < 16; y++){
        for(size_t x = 0; x < 16; x++){
            pixels[y][x] = (MC_AColor){
                .alpha = 1,
                .color = {
                    .r = 1,
                    .g = 1,
                    .b = 1
                }
            };
        }
    }

    MC_GBuffer *buffer;
    MC_Size2U size;

    MC_REQUIRE(mc_graphics_get_size(g, &size));
    MC_REQUIRE(mc_graphics_create_buffer(g, &buffer, size));
    MC_REQUIRE(mc_graphics_select_buffer(g, buffer));

    MC_REQUIRE(mc_graphics_begin(g));
    MC_REQUIRE(mc_graphics_clear(g, (MC_Color){0.2,0.1,0.05}));
    MC_REQUIRE(mc_graphics_write_pixels(g, (MC_Point2I){.x = 100, .y = 100}, (MC_Size2U){.width = 16, .height = 16}, (void*)pixels, (MC_Point2I){0, 0}));
    MC_REQUIRE(mc_graphics_end(g));

    MC_Stream *f;
    MC_REQUIRE(mc_fopen(&f, MC_STRC("dump.bmp"), MC_OPEN_WRITE));
    MC_REQUIRE(mc_graphics_dump(g, f));
    mc_close(f);

    MC_REQUIRE(mc_graphics_select_buffer(g, NULL));

    while(true){
        MC_WMEvent event;
        if(MC_REQUIRE(mc_wm_poll_event(wm, &event)) != MCE_OK){
            mc_sleep(&(MC_Time){.nsec = 1000000});
            continue;
        }

        mc_fmt(MC_STDOUT, "event %s\n", mc_wm_event_type_str(event.type));

        switch (event.type){
        case MC_WME_WINDOW_REDRAW_REQUESTED:
            MC_REQUIRE(mc_graphics_begin(g));
            MC_REQUIRE(mc_graphics_clear(g, (MC_Color){0}));
            MC_REQUIRE(mc_graphics_write(g, (MC_Point2I){.x = 0, .y = 0}, size, buffer, (MC_Point2I){0, 0}));
            MC_REQUIRE(mc_graphics_end(g));
            break;
        }
    }

    mc_wm_destroy(wm);
}

