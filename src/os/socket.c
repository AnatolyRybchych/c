#include <mc/os/socket.h>

#include <errno.h>
#include <memory.h>
#include <malloc.h>
#include <mc/util/error.h>

#include "_fd.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

typedef struct SocketCtx SocketCtx;

struct SocketCtx{
    FdCtx fd_ctx;
    MC_String *local_addr;
    MC_String *remote_addr;
    MC_SocketEndpoint local;
    MC_SocketEndpoint remote;
};

static MC_Error endpoint_type(MC_SocketType socket_type, int *domain, int *type, int *protocol);
static MC_Error endpoint_sockaddr(MC_SocketEndpoint *endpoint, struct sockaddr *addr, socklen_t *addrlen);

static void socket_close(void *ctx);

MC_Error mc_socket_connect(MC_Stream **sock, const MC_SocketEndpoint *endpoint){
    SocketCtx ctx = {
        .remote = *endpoint
    };

    int domain, type, protocol;
    MC_RETURN_ERROR(endpoint_type(ctx.remote.type, &domain, &type, &protocol));

    struct sockaddr_storage addr;
    socklen_t addrlen;
    MC_RETURN_ERROR(endpoint_sockaddr(&ctx.remote, (struct sockaddr *)&addr, &addrlen));

    MC_Error status = fd_open(&ctx.fd_ctx, socket(domain, type, protocol));
    if(status != MCE_OK){
        fd_close(&ctx.fd_ctx);
        return mc_error_from_errno(errno);
    }

    if(connect(ctx.fd_ctx.fd, (struct sockaddr *)&addr, addrlen)){
        fd_close(&ctx.fd_ctx);
        return mc_error_from_errno(errno);
    }

    MC_StreamVtab vtab = vtbl_fd;
    vtab.close = socket_close;

    status = mc_open(NULL, sock, &vtab, sizeof(SocketCtx), &ctx);
    if(status != MCE_OK){
        fd_close(&ctx.fd_ctx);
        return status;
    }

    return MCE_OK;
}

MC_Error mc_socket_listen(MC_Stream **sock, const MC_SocketEndpoint *endpoint, unsigned queue){
    SocketCtx ctx = {
        .local = *endpoint
    };

    int domain, type, protocol;
    MC_RETURN_ERROR(endpoint_type(ctx.local.type, &domain, &type, &protocol));

    struct sockaddr_storage addr;
    socklen_t addrlen;
    MC_RETURN_ERROR(endpoint_sockaddr(&ctx.local, (struct sockaddr *)&addr, &addrlen));

    MC_Error status = fd_open(&ctx.fd_ctx, socket(domain, type, protocol));
    if(status != MCE_OK){
        fd_close(&ctx.fd_ctx);
        return mc_error_from_errno(errno);
    }

    if(bind(ctx.fd_ctx.fd, (struct sockaddr*)&addr, addrlen)){
        fd_close(&ctx.fd_ctx);
        return mc_error_from_errno(errno);
    }

    if(listen(ctx.fd_ctx.fd, queue)){
        fd_close(&ctx.fd_ctx);
        return mc_error_from_errno(errno);
    }

    MC_StreamVtab vtab = vtbl_fd;
    vtab.close = socket_close;

    status = mc_open(NULL, sock, &vtab, sizeof(SocketCtx), &ctx);
    if(status != MCE_OK){
        fd_close(&ctx.fd_ctx);
        return status;
    }

    return MCE_OK;
}

MC_Error mc_socket_accept(MC_Stream **client, MC_Stream *sock){
    SocketCtx *ctx = mc_ctx(sock);
    SocketCtx client_ctx = {
        .local = ctx->local
    };

    struct sockaddr_storage addr;
    socklen_t addrlen;

    int client_fd = accept(ctx->fd_ctx.fd, (struct sockaddr*)&addr, &addrlen);
    if(client_fd < 0){
        return mc_error_from_errno(errno);
    }

    MC_Error status = fd_open(&client_ctx.fd_ctx, client_fd);
    if(status != MCE_OK){
        fd_close(&client_ctx.fd_ctx);
        return mc_error_from_errno(errno);
    }

    MC_StreamVtab vtab = vtbl_fd;
    vtab.close = socket_close;

    status = mc_open(NULL, client, &vtab, sizeof(SocketCtx), &client_ctx);
    if(status != MCE_OK){
        fd_close(&client_ctx.fd_ctx);
        return status;
    }

    return MCE_OK;
}

static MC_Error endpoint_type(MC_SocketType socket_type, int *domain, int *type, int *protocol){
    switch (socket_type){
    case MC_SOCKET_IPV4 | MC_SOCKET_DGRAM:
        *domain = AF_INET;
        *type = SOCK_DGRAM;
        *protocol = 0;
        return MCE_OK;
    case MC_SOCKET_IPV4 | MC_SOCKET_STREAM:
        *domain = AF_INET;
        *type = SOCK_STREAM;
        *protocol = 0;
        return MCE_OK;
    default:
        return MCE_INVALID_INPUT;
    }
}

static MC_Error endpoint_sockaddr(MC_SocketEndpoint *endpoint, struct sockaddr *addr, socklen_t *addrlen){
    switch (endpoint->type){
    case MC_SOCKET_IPV4 | MC_SOCKET_DGRAM:{
        struct sockaddr_in *a = (void*)addr;
        *addrlen = sizeof(struct sockaddr_in);
        *a = (struct sockaddr_in){
            .sin_family = AF_INET,
            .sin_port = htons(endpoint->transport.as.udp.port)
        };

        memcpy(&a->sin_addr, &endpoint->network.as.ipv4, sizeof(endpoint->network.as.ipv4));
    } return MCE_OK;
    case MC_SOCKET_IPV4 | MC_SOCKET_STREAM:{
        struct sockaddr_in *a = (void*)addr;
        *addrlen = sizeof(struct sockaddr_in);
        *a = (struct sockaddr_in){
            .sin_family = AF_INET,
            .sin_port = htons(endpoint->transport.as.udp.port)
        };

        memcpy(&a->sin_addr, &endpoint->network.as.ipv4, sizeof(endpoint->network.as.ipv4));
    } return MCE_OK;
    default: return MCE_INVALID_INPUT;
    }
}

static void socket_close(void *ctx){
    SocketCtx *sock_ctx = ctx;
    fd_close(sock_ctx);
    if(sock_ctx->local_addr){
        mc_free(NULL, sock_ctx->local_addr);
    }

    if(sock_ctx->remote_addr){
        mc_free(NULL, sock_ctx->remote_addr);
    }
}
