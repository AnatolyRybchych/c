#ifndef MC_NET_ENDPOINT_H
#define MC_NET_ENDPOINT_H

#include <mc/util/enum.h>
#include <mc/net/address.h>

#define MC_ITER_ENDPOINT_TYPE(CB, ...) \
    CB(ETHERNET, ##__VA_ARGS__) \
    CB(IPV4, ##__VA_ARGS__) \
    CB(IPV6, ##__VA_ARGS__) \
    CB(TCP, ##__VA_ARGS__) \
    CB(UDP, ##__VA_ARGS__) \

typedef unsigned MC_EndpointType;
enum MC_EndpointType{
    MC_ITER_ENDPOINT_TYPE(MC_DEFINE_ENUM_ITEM, MC_ENDPOINT_)
};

typedef struct MC_Endpoint MC_Endpoint;

typedef struct MC_UDPEndpoint MC_UDPEndpoint;
typedef struct MC_TCPEndpoint MC_TCPEndpoint;

struct MC_UDPEndpoint{
    MC_Address address;
    uint16_t port;
};

struct MC_TCPEndpoint{
    MC_Address address;
    uint16_t port;
};

struct MC_Endpoint{
    MC_EndpointType type;
    union{
        MC_EthernetAddress ether;
        MC_IPv4Address ipv4;
        MC_IPv6Address ipv6;
        MC_UDPEndpoint udp;
        MC_TCPEndpoint tcp;
    };
};

const char *mc_endpoint_type_str(MC_EndpointType type);
MC_Error mc_endpoint_to_string(const MC_Endpoint *endpoint, size_t bufsz, char *buf, size_t *written);
MC_Error mc_endpoint_write(MC_Stream *out, const MC_Endpoint *endpoint);
MC_Error mc_endpoint_parse(MC_Endpoint *endpoint, MC_Str str, MC_Str *match);
MC_Error mc_endpoint_parsec(MC_Endpoint *endpoint, const char *str, MC_Str *match);

#endif // MC_NET_ENDPOINT_H
