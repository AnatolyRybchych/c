#include <mc/os/socket.h>

#include <errno.h>
#include <memory.h>
#include <malloc.h>
#include <mc/util/error.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <poll.h>
#include <unistd.h>

typedef struct SocketCtx SocketCtx;
typedef unsigned Flags;
enum Flags {
    // socket has been created
    FLAG_CREATE     = 1 << 0,

    // ready for IO
    FLAG_CONNECT    = 1 << 1,

    // has remote endpoint info
    FLAG_REMOTE     = 1 << 2,

    // listen socket
    FLAG_LISTEN     = 1 << 3,

    FLAG_BIND       = 1 << 4,

    // has local endpoint info
    FLAG_LOCAL      = 1 << 5,

    // connection established
    FLAG_CONNECTED  = 1 << 6,

    FLAG_ASYNC  = 1 << 6,

    FLAG_REUSE_ADDRESS  = 1 << 7,

    FLAG_REUSE_PORT  = 1 << 8,
};

struct SocketCtx{
    int fd;
    MC_Endpoint local_endpoint;
    MC_Endpoint remote_endpoint;
    Flags flags;
};

static MC_Error sock_read(void *ctx, size_t size, void *data, size_t *read_size);
static MC_Error sock_write(void *ctx, size_t size, const void *data, size_t *written);
static void sock_close(void *ctx);

static MC_Error set_sock_flags(SocketCtx *ctx);
static MC_Error finish_async_connect(SocketCtx *ctx);

const MC_StreamVtab vtbl = {
    .read = sock_read,
    .write = sock_write,
    .close = sock_close,
};

static MC_Error read_endpoint(const MC_Endpoint *endpoint,
    int *domain, int *type, int *protocol, struct sockaddr *addr, socklen_t *addrlen);

MC_Error mc_socket(MC_Stream **sock, MC_SocketFlags flags) {
    SocketCtx ctx = {0};

    if(flags & MC_SOCKET_ASYNC)
        ctx.flags |= FLAG_ASYNC;

    if(flags & MC_SOCKET_REUSE_ADDRESS)
        ctx.flags |= FLAG_REUSE_ADDRESS;

    if(flags & MC_SOCKET_REUSE_PORT)
        ctx.flags |= FLAG_REUSE_PORT;

    return mc_open(NULL, sock, &vtbl, sizeof ctx, &ctx);
}

MC_Error mc_socket_bind(MC_Stream *sock, const MC_Endpoint *endpoint) {
    MC_RETURN_INVALID(sock == NULL);
    SocketCtx *ctx = mc_ctx(sock);

    int domain, type, protocol;
    struct sockaddr_storage addr;
    socklen_t addrlen;
    MC_RETURN_ERROR(read_endpoint(endpoint, &domain, &type, &protocol, (struct sockaddr*)&addr, &addrlen));

    if (!(ctx->flags & FLAG_CREATE)) {
        ctx->fd = socket(domain, type, protocol);
        if(ctx->fd < 0) {
            return mc_error_from_errno(errno);
        }

        ctx->flags |= FLAG_CREATE;

        MC_RETURN_ERROR(set_sock_flags(ctx));
    }

    if(!(ctx->flags & FLAG_BIND)) {
        if(bind(ctx->fd, (struct sockaddr*)&addr, addrlen)) {
            return mc_error_from_errno(errno);
        }

        ctx->local_endpoint = *endpoint;
        ctx->flags |= FLAG_BIND;
        ctx->flags |= FLAG_LOCAL;
    }
    else if(memcmp(&ctx->local_endpoint, endpoint, sizeof *endpoint)) {
        return MCE_BUSY;
    }

    return MCE_OK;
}

MC_Error mc_socket_connect(MC_Stream *sock, const MC_Endpoint *endpoint) {
    MC_RETURN_INVALID(sock == NULL);
    SocketCtx *ctx = mc_ctx(sock);

    int domain, type, protocol;
    struct sockaddr_storage addr;
    socklen_t addrlen;
    MC_RETURN_ERROR(read_endpoint(endpoint, &domain, &type, &protocol, (struct sockaddr*)&addr, &addrlen));

    if (!(ctx->flags & FLAG_CREATE)) {
        ctx->fd = socket(domain, type, protocol);
        if(ctx->fd < 0) {
            return mc_error_from_errno(errno);
        }

        ctx->flags |= FLAG_CREATE;

        MC_RETURN_ERROR(set_sock_flags(ctx));
    }

    if(!(ctx->flags & FLAG_CONNECT)) {
        if(connect(ctx->fd, (struct sockaddr*)&addr, addrlen)) {
            if(errno != EINPROGRESS){
                return mc_error_from_errno(errno);
            }
        }
        else {
            ctx->flags |= FLAG_CONNECTED;
        }

        ctx->remote_endpoint = *endpoint;
        ctx->flags |= FLAG_CONNECT;
        ctx->flags |= FLAG_REMOTE;
    }
    else if(memcmp(&ctx->local_endpoint, endpoint, sizeof *endpoint)) {
        return MCE_BUSY;
    }

    return MCE_OK;
}

MC_Error mc_socket_listen(MC_Stream *socket, unsigned queue) {
    MC_RETURN_INVALID(socket == NULL);
    SocketCtx *ctx = mc_ctx(socket);

    MC_RETURN_INVALID(!(ctx->flags & FLAG_CREATE));
    MC_RETURN_INVALID(!(ctx->flags & FLAG_BIND));

    if(listen(ctx->fd, queue)) {
        return mc_error_from_errno(errno);
    }

    return MCE_OK;
}

MC_Error mc_socket_accept(MC_Stream *socket, MC_Stream **client) {
    MC_RETURN_INVALID(socket == NULL);
    SocketCtx *ctx = mc_ctx(socket);

    MC_RETURN_INVALID(!(ctx->flags & FLAG_CREATE));
    MC_RETURN_INVALID(!(ctx->flags & FLAG_BIND));

    struct sockaddr_storage addr;
    socklen_t addrlen;
    int client_fd = accept(ctx->fd, (struct sockaddr*)&addr, &addrlen);
    if(client_fd < 0) {
        return mc_error_from_errno(errno);
    }

    SocketCtx client_ctx = {
        .fd = client_fd,
        .flags = FLAG_CREATE | FLAG_BIND | FLAG_LOCAL | FLAG_CONNECT | FLAG_CONNECTED,
        .local_endpoint = ctx->local_endpoint,
        .remote_endpoint = ctx->remote_endpoint,
    };

    return mc_open(NULL, client, &vtbl, sizeof client_ctx, &client_ctx);
}

static MC_Error read_endpoint(const MC_Endpoint *endpoint,
    int *domain, int *type, int *protocol, struct sockaddr *addr, socklen_t *addrlen)
{
    MC_RETURN_INVALID(endpoint == NULL);
    if(endpoint->type == MC_ENDPOINT_UDP) {
        *type = SOCK_DGRAM;
        *protocol = 0;

        if(endpoint->udp.address.type == MC_ADDRTYPE_IPV4) {
            *domain = AF_INET;

            struct sockaddr_in in_addr = {
                .sin_port = htons(endpoint->udp.port),
                .sin_family = AF_INET
            };

            memcpy(&in_addr.sin_addr, endpoint->udp.address.ipv4.data, sizeof endpoint->udp.address.ipv4.data);
            *addrlen = sizeof in_addr;
            memcpy(addr, &in_addr, sizeof in_addr);
        }
        else if(endpoint->udp.address.type == MC_ADDRTYPE_IPV6){
            *domain = AF_INET6;
            struct sockaddr_in6 in6_addr = {
                .sin6_family = AF_INET6,
                .sin6_port = htons(endpoint->udp.port)
            };

            memcpy(&in6_addr.sin6_addr, endpoint->udp.address.ipv6.data, sizeof endpoint->udp.address.ipv6.data);
            *addrlen = sizeof in6_addr;
            memcpy(addr, &in6_addr, sizeof in6_addr);
        }
        else{
            return MCE_INVALID_INPUT;
        }
    }
    else if(endpoint->type == MC_ENDPOINT_TCP) {
        *type = SOCK_STREAM;
        *protocol = 0;

        if(endpoint->udp.address.type == MC_ADDRTYPE_IPV4) {
            *domain = AF_INET;

            struct sockaddr_in in_addr = {
                .sin_port = htons(endpoint->udp.port),
                .sin_family = AF_INET
            };

            memcpy(&in_addr.sin_addr, endpoint->udp.address.ipv4.data, sizeof endpoint->udp.address.ipv4.data);
            *addrlen = sizeof in_addr;
            memcpy(addr, &in_addr, sizeof in_addr);
        }
        else if(endpoint->udp.address.type == MC_ADDRTYPE_IPV6){
            *domain = AF_INET6;
            struct sockaddr_in6 in6_addr = {
                .sin6_family = AF_INET6,
                .sin6_port = htons(endpoint->udp.port)
            };

            memcpy(&in6_addr.sin6_addr, endpoint->udp.address.ipv6.data, sizeof endpoint->udp.address.ipv6.data);
            *addrlen = sizeof in6_addr;
            memcpy(addr, &in6_addr, sizeof in6_addr);
        }
        else{
            return MCE_INVALID_INPUT;
        }
    }
    else {
        return MCE_NOT_IMPLEMENTED;
    }

    return MCE_OK;
}

static MC_Error sock_read(void *_ctx, size_t size, void *data, size_t *read_size) {
    SocketCtx *ctx = _ctx;
    *read_size = 0;

    MC_RETURN_INVALID(!(ctx->flags & FLAG_CREATE));
    MC_RETURN_INVALID(!(ctx->flags & FLAG_CONNECT));

    MC_RETURN_ERROR(finish_async_connect(ctx));

    ssize_t sz_read = 0;
    while (true) {
        ssize_t cur_read = read(ctx->fd, data, size - sz_read);
        if(cur_read <= 0) {
            break;
        }

        sz_read += cur_read;
    }

    *read_size = sz_read;
    return mc_error_from_errno(errno);
}

static MC_Error sock_write(void *_ctx, size_t size, const void *data, size_t *written) {
    SocketCtx *ctx = _ctx;
    *written = 0;

    MC_RETURN_INVALID(!(ctx->flags & FLAG_CREATE));
    MC_RETURN_INVALID(!(ctx->flags & FLAG_CONNECT));

    MC_RETURN_ERROR(finish_async_connect(ctx));

    ssize_t sz_written = write(ctx->fd, data, size);
    if(sz_written <= 0){
        *written = 0;
        return mc_error_from_errno(errno);
    }

    *written = sz_written;
    return MCE_OK;
}

static void sock_close(void *_ctx) {
    SocketCtx *ctx = _ctx;

    if(ctx->flags | FLAG_CREATE) {
        close(ctx->fd);
    }
}

static MC_Error set_sock_flags(SocketCtx *ctx) {
    int flags = fcntl(ctx->fd, F_GETFL, 0);
    if (flags == -1) {
        return mc_error_from_errno(errno);
    }

    if (ctx->flags & FLAG_ASYNC) {
        flags |= O_NONBLOCK;
    }

    if (ctx->flags & FLAG_REUSE_ADDRESS
    && setsockopt(ctx->fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) {
        return mc_error_from_errno(errno);
    }

    if (ctx->flags & FLAG_REUSE_PORT
    && setsockopt(ctx->fd, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int))) {
        return mc_error_from_errno(errno);
    }

    if (fcntl(ctx->fd, F_SETFL, flags) == -1) {
        return mc_error_from_errno(errno);
    }

    return MCE_OK;
}

static MC_Error finish_async_connect(SocketCtx *ctx){
    if(ctx->flags & FLAG_CONNECT && !(ctx->flags & FLAG_CONNECTED)) {
        struct pollfd pfd;
        pfd.fd = ctx->fd;
        pfd.events = POLLOUT;

        int events = poll(&pfd, 1, 0);
        if(events < 0) {
            return mc_error_from_errno(errno);
        }
        else if(events == 0) {
            return MCE_AGAIN;
        }
        else{
            ctx->flags |= FLAG_CONNECTED;
        }
    }

    return MCE_OK;
}


