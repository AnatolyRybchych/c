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
#include <mc/os/socket.h>
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
    MC_Stream *file;
    MC_REQUIRE(mc_fopen(&file, MC_STRC("/dev/urandom"), MC_OPEN_READ | MC_OPEN_ASYNC));

    char buffer[256] = {0};
    size_t read;
    while(true){
        MC_REQUIRE(mc_read_async(file, sizeof(buffer) - 1, buffer, &read));
        if(read){
            MC_REQUIRE(mc_write(MC_STDOUT, read, buffer, NULL));
        }
    }


    mc_close(file);
}

