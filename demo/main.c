#include <mc/util/assert.h>
#include <mc/util/minmax.h>
#include <mc/sched.h>
#include <mc/data/string.h>
#include <stdlib.h>

#include <mc/net/endpoint.h>
#include <mc/os/socket.h>

int main(){
    MC_Stream *server;
    MC_REQUIRE(mc_socket(&server, MC_SOCKET_ASYNC | MC_SOCKET_REUSE_PORT));
    MC_REQUIRE(mc_socket_bind(server, &(MC_Endpoint){
        .type = MC_ENDPOINT_TCP,
        .tcp = {
            .port = 4321,
            .address = {
                .type = MC_ADDRTYPE_IPV4,
                .ipv4.data = {127, 0, 0, 1}
            }
        }
    }));

    MC_REQUIRE(mc_socket_listen(server, 10));

    MC_Stream *client;

    while (MC_REQUIRE(mc_socket_accept(server, &client)) == MCE_AGAIN);

    MC_REQUIRE(mc_fmt(client, "test\n"));

    mc_close(client);
    mc_close(server);
}

