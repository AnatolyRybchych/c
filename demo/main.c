#include <mc/util/assert.h>
#include <mc/util/minmax.h>
#include <mc/sched.h>
#include <mc/data/string.h>
#include <stdlib.h>

#include <mc/net/endpoint.h>
#include <mc/os/socket.h>

#include <mc/net/proto/http.h>
#include <mc/data/list.h>

int main(){
    MC_Endpoint endpoint;
    MC_REQUIRE(mc_endpoint_parsec(&endpoint, "tcp://127.0.0.1:8000", NULL));
    MC_REQUIRE(mc_endpoint_write(MC_STDOUT, &endpoint));
    MC_REQUIRE(mc_fmt(MC_STDOUT, "\n"));

    MC_Stream *server;
    MC_REQUIRE(mc_socket(&server, MC_SOCKET_ASYNC | MC_SOCKET_REUSE_PORT));
    MC_REQUIRE(mc_socket_bind(server, &endpoint));

    MC_REQUIRE(mc_socket_listen(server, 10));

    MC_Stream *client;

    while (MC_REQUIRE(mc_socket_accept(server, &client)) == MCE_AGAIN);

    MC_HttpReader *reader;
    MC_REQUIRE(mc_http_reader_new(NULL, &reader, client));

    const MC_HttpMessage *msg;
    while (MC_REQUIRE(mc_http_reader_read(reader, &msg, MC_STDOUT)) == MCE_AGAIN) mc_sleep(&(MC_Time){.nsec = 1000000});

    MC_HttpComposer *composer;
    MC_REQUIRE(mc_http_composer_new(NULL, &composer));
    MC_REQUIRE(mc_http_composer_begin_response(composer));
    MC_REQUIRE(mc_http_composer_set_version(composer, (MC_HttpVersion){.major = 1, .minor = 1}));
    MC_REQUIRE(mc_http_composer_get_message(composer, &msg));


    MC_HttpWriter *writer;
    MC_REQUIRE(mc_http_writer_new(NULL, &writer));
    MC_REQUIRE(mc_http_writer_beg(writer, client, msg));

    MC_REQUIRE(mc_http_writer_write(writer));

    mc_close(client);
    mc_close(server);
}

