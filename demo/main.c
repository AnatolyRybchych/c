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
    MC_Stream *server;
    MC_REQUIRE(mc_socket_listen(&server, &(MC_SocketEndpoint){
        .transport = {
            .network = {
                .type = MC_SOCKET_IPV4 | MC_SOCKET_STREAM,
                .as.ipv4.addr = {0, 0, 0, 0},
            },
            .as.tcp.port = 4321
        }
    }, 10));

    MC_Stream *client;
    MC_REQUIRE(mc_socket_accept(&client, server));

    char buf[32] = {0};
    size_t read;
    MC_REQUIRE(mc_read_async(client, 31, buf, &read));

    mc_fmt(MC_STDOUT, "%s\n", buf);

    mc_close(client);
    mc_close(server);
}

