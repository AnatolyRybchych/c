#ifndef MC_OS_SOCKET_H
#define MC_OS_SOCKET_H

#include <mc/error.h>
#include <mc/data/str.h>
#include <mc/io/stream.h>

#include <stdint.h>

#define ITER_SOCKET_LINK() \
    SOCKET_LINK(ETHER)

#define ITER_SOCKET_NETWORK() \
    SOCKET_NETWORK(IPV4) \
    SOCKET_NETWORK(IPv6) \
    // SOCKET_NETWORK(UNIX)

#define ITER_SOCKET_TRANSPORT() \
    SOCKET_TRANSPORT(STREAM) \
    SOCKET_TRANSPORT(DGRAM) \

typedef uint32_t MC_SocketType;
enum MC_SocketType{
    MC_SOCKET_LINK_BEG = 0x0,
    #define SOCKET_LINK(NAME) MC_SOCKET_##NAME,
    ITER_SOCKET_LINK()
    #undef SOCKET_LINK
    MC_SOCKET_LINK = 0xFF,

    MC_SOCKET_NETWORK_BEG = 0xFF,
    #define SOCKET_NETWORK(NAME) MC_SOCKET_##NAME,
    ITER_SOCKET_NETWORK()
    #undef SOCKET_NETWORK
    MC_SOCKET_NETWORK = 0x00FF,

    MC_SOCKET_TRANSPORT_BEG = 0xFFFF,
    #define SOCKET_TRANSPORT(NAME) MC_SOCKET_##NAME,
    ITER_SOCKET_TRANSPORT()
    #undef SOCKET_TRANSPORT
    MC_SOCKET_TRANSPORT = 0x0000FF,
};

typedef struct MC_MAC MC_MAC;
typedef struct MC_SocketLink MC_SocketLink;
typedef struct MC_IPv4 MC_IPv4;
typedef struct MC_IPv6 MC_IPv6;
typedef struct MC_SocketNetwork MC_SocketNetwork;
typedef struct MC_SocketTransport MC_SocketTransport;
typedef struct MC_SocketEndpoint MC_SocketEndpoint;

MC_Error mc_socket_connect(MC_Stream **socket, const MC_SocketEndpoint *endpoint);
// MC_Error mc_socket_listen(MC_Stream **socket, const MC_SocketEndpoint *endpoint, unsigned queue);
// MC_Error mc_socket_accept(MC_Stream **client, MC_Stream *socket);

struct MC_MAC{
    uint8_t addr[6];
};

struct MC_SocketLink{
    MC_SocketType type;
    union{
        MC_MAC ether;
    } as;
};

struct MC_IPv4{
    uint8_t addr[4];
};

struct MC_IPv6{
    uint8_t addr[16];
};

struct MC_SocketNetwork{
    union{
        MC_SocketType type;
        MC_SocketLink link;
    };
    
    union{
        MC_IPv4 ipv4;
        MC_IPv6 ipv6;
        MC_Str unix_path;
    } as;
};

struct MC_SocketTransport{
    union{
        MC_SocketType type;
        MC_SocketNetwork network;
    };

    union{
        struct{
            uint16_t port;
        } tcp;

        struct{
            uint16_t port;
        } udp;
    } as;
};

struct MC_SocketEndpoint{
    union{
        MC_SocketType type;
        MC_SocketNetwork link;
        MC_SocketNetwork network;
        MC_SocketTransport transport;
    };
};

#endif // MC_OS_SOCKET_H
