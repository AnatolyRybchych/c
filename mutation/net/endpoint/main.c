#include <mc/os/file.h>
#include <mc/net/endpoint.h>
#include <mc/util/assert.h>

#define L(...) (mc_fmt(MC_STDOUT, "\n%s\n", #__VA_ARGS__), __VA_ARGS__)
#define LR(...) MC_REQUIRE(L(__VA_ARGS__))

int main(){
    LR(mc_endpoint_write(MC_STDOUT, &(MC_Endpoint){
        .type = MC_ENDPOINT_ETHERNET,
        .ether.data = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66}
    }));
    mc_fmt(MC_STDOUT, "\n");

    LR(mc_endpoint_write(MC_STDOUT, &(MC_Endpoint){
        .type = MC_ENDPOINT_IPV4,
        .ipv4.data = {192, 168, 1, 1}
    }));
    mc_fmt(MC_STDOUT, "\n");

    LR(mc_endpoint_write(MC_STDOUT, &(MC_Endpoint){
        .type = MC_ENDPOINT_IPV6,
        .ipv6.data = {0x20, 0x00}
    }));
    mc_fmt(MC_STDOUT, "\n");

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
    mc_fmt(MC_STDOUT, "\n");

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
    mc_fmt(MC_STDOUT, "\n");
}
