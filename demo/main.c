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
#include <mc/os/process.h>

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
    mc_fmt(MC_STDOUT, "TEST\n");
    mc_sleep(&(MC_Time){.sec = 1});
    MC_Process *proc;
    MC_REQUIRE(mc_process_program(&proc, MC_STRC("/home/anatolii/Desktop/c/bin/demo"), 0, NULL));
    mc_process_wait(proc);
}

