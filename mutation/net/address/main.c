#include <mc/os/file.h>
#include <mc/net/address.h>
#include <mc/util/assert.h>

#define L(...) (mc_fmt(MC_STDOUT, "%s\n", #__VA_ARGS__), __VA_ARGS__)
#define LR(...) MC_REQUIRE(L(__VA_ARGS__))
#define DELIM() mc_fmt(MC_STDOUT, "================================\n\n");
#define NL() mc_fmt(MC_STDOUT, "\n");

int main(){
    LR(mc_address_write(MC_STDOUT, &(MC_Address){
        .type = MC_ADDRTYPE_ETHERNET,
        .ether.data = {0, 0, 0, 0, 0, 0}
    }));
    NL();
    DELIM();

    LR(mc_address_write(MC_STDOUT, &(MC_Address){
        .type = MC_ADDRTYPE_ETHERNET,
        .ether.data = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}
    }));
    NL();
    DELIM();

    LR(mc_address_write(MC_STDOUT, &(MC_Address){
        .type = MC_ADDRTYPE_IPV4,
        .ipv4.data = {192, 168, 1, 1}
    }));
    NL();
    DELIM();

    LR(mc_address_write(MC_STDOUT, &(MC_Address){
        .type = MC_ADDRTYPE_IPV6,
        .ipv6.data = {0x20, 0x00}
    }));
    NL();
    DELIM();

    LR(mc_address_write(MC_STDOUT, &(MC_Address){
        .type = MC_ADDRTYPE_IPV6,
        .ipv6.data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}
    }));
    NL();
    DELIM();

    LR(mc_address_write(MC_STDOUT, &(MC_Address){
        .type = MC_ADDRTYPE_IPV6,
        .ipv6.data = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 192, 168, 1, 1}
    }));
    NL();
    DELIM();

    MC_Address address;
    MC_Str match;
    LR(mc_address_parsec(&address, "192.168.1.1", &match));
    LR(mc_address_write(MC_STDOUT, &address));
    NL();
    LR(mc_fmt(MC_STDOUT, "%.*s\n", match.end - match.beg, match.beg));
    DELIM()

    LR(mc_address_parsec(&address, "::ffff:192.168.1.1", &match));
    LR(mc_address_write(MC_STDOUT, &address));
    NL();
    LR(mc_fmt(MC_STDOUT, "%.*s\n", match.end - match.beg, match.beg));
    DELIM()

    LR(mc_address_parsec(&address, "2000::fad1", &match));
    LR(mc_address_write(MC_STDOUT, &address));
    NL();
    LR(mc_fmt(MC_STDOUT, "%.*s\n", match.end - match.beg, match.beg));
    DELIM()

    LR(mc_address_parsec(&address, "11:22:33:44:55:66", &match));
    LR(mc_address_write(MC_STDOUT, &address));
    NL();
    LR(mc_fmt(MC_STDOUT, "%.*s\n", match.end - match.beg, match.beg));
    DELIM()
}
