#include <mc/os/file.h>
#include <mc/net/endpoint.h>
#include <mc/util/assert.h>

#define L(...) (mc_fmt(MC_STDOUT, "%s\n", #__VA_ARGS__), __VA_ARGS__)
#define LR(...) MC_REQUIRE(L(__VA_ARGS__))
#define DELIM() mc_fmt(MC_STDOUT, "================================\n\n");
#define NL() mc_fmt(MC_STDOUT, "\n");

int main(){
    LR(mc_endpoint_write(MC_STDOUT, &(MC_Endpoint){
        .type = MC_ENDPOINT_ETHERNET,
        .ether.data = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66}
    }));
    NL();
    DELIM();

    LR(mc_endpoint_write(MC_STDOUT, &(MC_Endpoint){
        .type = MC_ENDPOINT_IPV4,
        .ipv4.data = {192, 168, 1, 1}
    }));
    NL();
    DELIM();

    LR(mc_endpoint_write(MC_STDOUT, &(MC_Endpoint){
        .type = MC_ENDPOINT_IPV6,
        .ipv6.data = {0x20, 0x00}
    }));
    NL();
    DELIM();

    LR(mc_endpoint_write(MC_STDOUT, &(MC_Endpoint){
        .type = MC_ENDPOINT_UDP,
        .udp = {
            .address = {
                .type = MC_ADDRTYPE_IPV4,
                .ipv4.data = {192, 168, 1, 1}
            },
            .port = 1234
        }
    }));
    NL();
    DELIM();

    LR(mc_endpoint_write(MC_STDOUT, &(MC_Endpoint){
        .type = MC_ENDPOINT_TCP,
        .tcp = {
            .address = {
                .type = MC_ADDRTYPE_IPV6,
                .ipv6.data = {0x20,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1}
            },
            .port = 1234
        }
    }));
    NL();
    DELIM();

    MC_Endpoint endpoint;

    LR(mc_endpoint_parsec(&endpoint, "11:22:33:44:55:fF", NULL));
    LR(mc_endpoint_write(MC_STDOUT, &endpoint));
    NL();
    DELIM();

    LR(mc_endpoint_parsec(&endpoint, "127.0.0.1", NULL));
    LR(mc_endpoint_write(MC_STDOUT, &endpoint));
    NL();
    DELIM();

    LR(mc_endpoint_parsec(&endpoint, "2001::1", NULL));
    LR(mc_endpoint_write(MC_STDOUT, &endpoint));
    NL();
    DELIM();

    LR(mc_endpoint_parsec(&endpoint, "tcp://192.168.1.1:8000", NULL));
    LR(mc_endpoint_write(MC_STDOUT, &endpoint));
    NL();
    DELIM();

    LR(mc_endpoint_parsec(&endpoint, "udp://8.8.8.8:67", NULL));
    LR(mc_endpoint_write(MC_STDOUT, &endpoint));
    NL();
    DELIM();

    LR(mc_endpoint_parsec(&endpoint, "udp://[::]:67", NULL));
    LR(mc_endpoint_write(MC_STDOUT, &endpoint));
    NL();
    DELIM();
}
