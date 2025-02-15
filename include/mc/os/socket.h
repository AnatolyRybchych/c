#ifndef MC_OS_SOCKET_H
#define MC_OS_SOCKET_H

#include <mc/error.h>
#include <mc/data/str.h>
#include <mc/data/stream.h>
#include <mc/net/endpoint.h>

#include <stdint.h>

MC_Error mc_socket(MC_Stream **socket);
MC_Error mc_socket_bind(MC_Stream *socket, const MC_Endpoint *endpoint);
MC_Error mc_socket_connect(MC_Stream *socket, const MC_Endpoint *endpoint);
MC_Error mc_socket_listen(MC_Stream *socket, unsigned queue);
MC_Error mc_socket_accept(MC_Stream *socket, MC_Stream **client);

#endif // MC_OS_SOCKET_H
