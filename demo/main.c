#include <mc/util/assert.h>
#include <mc/util/minmax.h>
#include <mc/sched.h>
#include <mc/data/string.h>
#include <stdlib.h>

#include <mc/net/endpoint.h>
#include <mc/os/socket.h>

int main(){
    MC_Endpoint endpoint;
    MC_REQUIRE(mc_endpoint_parsec(&endpoint, "tcp://127.0.0.1:4321", NULL));
    MC_REQUIRE(mc_endpoint_write(MC_STDOUT, &endpoint));
    MC_REQUIRE(mc_fmt(MC_STDOUT, "\n"));

    MC_Stream *server;
    MC_REQUIRE(mc_socket(&server, MC_SOCKET_ASYNC | MC_SOCKET_REUSE_PORT));
    MC_REQUIRE(mc_socket_bind(server, &endpoint));

    MC_REQUIRE(mc_socket_listen(server, 10));

    MC_Stream *client;

    while (MC_REQUIRE(mc_socket_accept(server, &client)) == MCE_AGAIN);

    MC_REQUIRE(mc_fmt(client, "test\n"));

    mc_close(client);
    mc_close(server);
}

