#include <mc/util/assert.h>
#include <mc/util/minmax.h>
#include <mc/sched.h>
#include <mc/data/string.h>
#include <stdlib.h>

#include <mc/net/endpoint.h>
#include <mc/os/socket.h>

int main(){
    // MC_Str match, oct1, oct2, oct3, oct4;
    // bool matches = mc_str_match(MC_STRC("192.168.1.1"), "(%d+)%.(%d+)%.(%d+)%.(%d+)",
    //     &match, &oct1, &oct2, &oct3, &oct4);

    // mc_fmt(MC_STDOUT, "mc_str_match -> %s\n", matches ? "true" : "false");
    // mc_fmt(MC_STDOUT, "%.*s\n", mc_str_len(match), match.beg);
    // mc_fmt(MC_STDOUT, "oct1 -> %.*s\n", oct1.end - oct1.beg, oct1.beg);
    // mc_fmt(MC_STDOUT, "oct2 -> %.*s\n", oct2.end - oct2.beg, oct2.beg);
    // mc_fmt(MC_STDOUT, "oct3 -> %.*s\n", oct3.end - oct3.beg, oct3.beg);
    // mc_fmt(MC_STDOUT, "oct4 -> %.*s\n", oct4.end - oct4.beg, oct4.beg);

    MC_Address address;
    MC_REQUIRE(mc_address_parse(&address, MC_STRC("127.0.0.1"), NULL));

    MC_Stream *server;
    MC_REQUIRE(mc_socket(&server, MC_SOCKET_ASYNC | MC_SOCKET_REUSE_PORT));
    MC_REQUIRE(mc_socket_bind(server, &(MC_Endpoint){
        .type = MC_ENDPOINT_TCP,
        .tcp = {
            .port = 4321,
            .address = address
        }
    }));

    MC_REQUIRE(mc_socket_listen(server, 10));

    MC_Stream *client;

    while (MC_REQUIRE(mc_socket_accept(server, &client)) == MCE_AGAIN);

    MC_REQUIRE(mc_fmt(client, "test\n"));

    mc_close(client);
    mc_close(server);
}

