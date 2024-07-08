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

#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

int main(){
    MC_WM *wm;
    MC_REQUIRE(mc_wm_init(&wm, mc_xlib_wm_vtab));

    MC_WMWindow *window;
    MC_REQUIRE(mc_wm_init_window(wm, &window));

    MC_Time exit_time;
    MC_REQUIRE(mc_gettime(MC_GETTIME_SINCE_BOOT, &exit_time));
    mc_timesum(&exit_time, &(MC_Time){.sec = 3}, &exit_time);

    while(true){
        MC_Time now;
        MC_REQUIRE(mc_gettime(MC_GETTIME_SINCE_BOOT, &now));
        if(mc_timecmp(&now, &exit_time) >= 0){
            return 0;
        }

        MC_WMEvent event;
        MC_REQUIRE(mc_wm_poll_event(wm, &event));
        MC_REQUIRE(mc_sleep(&(MC_Time){.nsec = 100000000}));
    }

    mc_wm_destroy(wm);
}

