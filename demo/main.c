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
    MC_Stream *sock;
    MC_REQUIRE(mc_socket_connect(&sock, &(MC_SocketEndpoint){
        .transport = {
            .network = {
                .type = MC_SOCKET_IPV4 | MC_SOCKET_STREAM,
                .as.ipv4.addr = {127, 0, 0, 1},
            },
            .as.tcp.port = 4321
        }
    }));

    mc_fmt(sock, "test123\n");
    mc_close(sock);
}

