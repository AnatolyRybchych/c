#ifndef MC_OS_SOCKET_H
#define MC_OS_SOCKET_H

#include <mc/error.h>
#include <mc/data/str.h>
#include <mc/data/stream.h>
#include <mc/net/endpoint.h>

#include <stdint.h>

typedef unsigned MC_SocketFlags;
enum MC_SocketFlags {
    MC_SOCKET_BASIC = 0,
    MC_SOCKET_ASYNC = 1 << 0,
    MC_SOCKET_REUSE_ADDRESS = 1 << 1,
    MC_SOCKET_REUSE_PORT = 1 << 2,
};

MC_Error mc_socket(MC_Stream **socket, MC_SocketFlags flags);
MC_Error mc_socket_bind(MC_Stream *socket, const MC_Endpoint *endpoint);
MC_Error mc_socket_connect(MC_Stream *socket, const MC_Endpoint *endpoint);
MC_Error mc_socket_listen(MC_Stream *socket, unsigned queue);
MC_Error mc_socket_accept(MC_Stream *socket, MC_Stream **client);

#endif // MC_OS_SOCKET_H
