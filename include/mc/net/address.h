#ifndef MC_NET_ADDRESS_H
#define MC_NET_ADDRESS_H

#include <mc/util/enum.h>
#include <mc/error.h>
#include <mc/data/stream.h>

#include <stddef.h>
#include <stdint.h>

#define MC_ITER_ADDRESS_TYPE(CB, ...) \
    CB(ETHERNET, ##__VA_ARGS__) \
    CB(IPV4, ##__VA_ARGS__) \
    CB(IPV6, ##__VA_ARGS__) \

typedef unsigned MC_AddressType;
enum MC_AddressType{
    MC_ITER_ADDRESS_TYPE(MC_DEFINE_ENUM_ITEM, MC_ADDRTYPE_)
};

typedef struct MC_Address MC_Address;
typedef struct MC_EthernetAddress MC_EthernetAddress;
typedef struct MC_IPv4Address MC_IPv4Address;
typedef struct MC_IPv6Address MC_IPv6Address;

struct MC_EthernetAddress{
    uint8_t data[6];
};

struct MC_IPv4Address{
    uint8_t data[4];
};

struct MC_IPv6Address{
    uint8_t data[16];
};

struct MC_Address {
    MC_AddressType type;
    union {
        MC_EthernetAddress ether;
        MC_IPv4Address ipv4;
        MC_IPv6Address ipv6;
    };
};

const char *mc_address_type_str(MC_AddressType type);
MC_Error mc_address_to_string(const MC_Address *address, size_t bufsz, char *buf, size_t *written);
MC_Error mc_address_write(MC_Stream *out, const MC_Address *address);

#endif // MC_NET_ADDRESS_H
