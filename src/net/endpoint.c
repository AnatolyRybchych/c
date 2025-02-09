#include <mc/net/endpoint.h>
#include <mc/data/mstream.h>
#include <mc/data/alloc/sarena.h>
#include <mc/util/error.h>
#include <mc/util/minmax.h>

#include <stdio.h>
#include <string.h>

static MC_Error validate_endpoint(const MC_Endpoint *endpoint);

static MC_Error tcp_endpoint_write(MC_Stream *out, const MC_TCPEndpoint *endpoint);
static MC_Error udp_endpoint_write(MC_Stream *out, const MC_UDPEndpoint *endpoint);

const char *mc_endpoint_type_str(MC_EndpointType type){
    const char *result = NULL;
    MC_ENUM_TO_STRING(result, type, MC_ITER_ENDPOINT_TYPE, MC_ENDPOINT_)
    return result;
}

MC_Error mc_endpoint_to_string(const MC_Endpoint *endpoint, size_t bufsz, char *buf, size_t *written){
    char buffer[512];
    MC_Sarena sarena;
    MC_Alloc *alloc = mc_sarena(&sarena, sizeof buffer, buffer);
    MC_Stream *out;
    MC_RETURN_ERROR(mc_mstream(alloc, &out));

    MC_Error error = mc_endpoint_write(out, endpoint);
    size_t out_size = mc_mstream_size(out);
    MC_RETURN_ERROR(mc_set_cursor(out, 0, MC_CURSOR_FROM_BEG));
    MC_RETURN_ERROR(mc_read(out, bufsz, buf, written));

    buf[MIN(bufsz - 1, out_size)] = 0;

    if(out_size >= bufsz) {
        return MCE_TRUNCATED;
    }

    return error;
}

MC_Error mc_endpoint_write(MC_Stream *out, const MC_Endpoint *endpoint){
    MC_RETURN_ERROR(validate_endpoint(endpoint));

    switch (endpoint->type){
    case MC_ENDPOINT_ETHERNET:
        return mc_address_write(out, &(MC_Address){
            .type = MC_ADDRTYPE_ETHERNET,
            .ether = endpoint->ether
        });
    case MC_ENDPOINT_IPV4:
        return mc_address_write(out, &(MC_Address){
            .type = MC_ADDRTYPE_IPV4,
            .ipv4 = endpoint->ipv4
        });
    case MC_ENDPOINT_IPV6:
        return mc_address_write(out, &(MC_Address){
            .type = MC_ADDRTYPE_IPV6,
            .ipv6 = endpoint->ipv6
        });
    case MC_ENDPOINT_TCP:
        return tcp_endpoint_write(out, &endpoint->tcp);
    case MC_ENDPOINT_UDP:
        return udp_endpoint_write(out, &endpoint->udp);
    default:
        return MCE_INVALID_INPUT;
    }
}

static MC_Error validate_endpoint(const MC_Endpoint *endpoint){
    switch (endpoint->type){
    case MC_ENDPOINT_ETHERNET:
        return MCE_OK;
    case MC_ENDPOINT_IPV4:
        return MCE_OK;
    case MC_ENDPOINT_IPV6:
        return MCE_OK;
    case MC_ENDPOINT_TCP:
        MC_RETURN_INVALID(endpoint->tcp.address.type != MC_ADDRTYPE_IPV4
            && endpoint->tcp.address.type != MC_ADDRTYPE_IPV6);
        return MCE_OK;
    case MC_ENDPOINT_UDP:
        MC_RETURN_INVALID(endpoint->udp.address.type != MC_ADDRTYPE_IPV4
            && endpoint->udp.address.type != MC_ADDRTYPE_IPV6);
        return MCE_OK;
    default:
        return MCE_INVALID_INPUT;
    }
}

static MC_Error tcp_endpoint_write(MC_Stream *out, const MC_TCPEndpoint *endpoint){
    MC_RETURN_ERROR(mc_fmt(out, "tcp://"));
    if(endpoint->address.type == MC_ADDRTYPE_IPV6) {
        MC_RETURN_ERROR(mc_fmt(out, "["));
        MC_RETURN_ERROR(mc_address_write(out, &endpoint->address));
        MC_RETURN_ERROR(mc_fmt(out, "]"));
    }
    else{
        MC_RETURN_ERROR(mc_address_write(out, &endpoint->address));
    }

    MC_RETURN_ERROR(mc_fmt(out, ":%u", (unsigned)endpoint->port));
    return MCE_OK;
}
static MC_Error udp_endpoint_write(MC_Stream *out, const MC_UDPEndpoint *endpoint){
    MC_RETURN_ERROR(mc_fmt(out, "udp://"));
    if(endpoint->address.type == MC_ADDRTYPE_IPV6) {
        MC_RETURN_ERROR(mc_fmt(out, "["));
        MC_RETURN_ERROR(mc_address_write(out, &endpoint->address));
        MC_RETURN_ERROR(mc_fmt(out, "]"));
    }
    else{
        MC_RETURN_ERROR(mc_address_write(out, &endpoint->address));
    }

    MC_RETURN_ERROR(mc_fmt(out, ":%u", (unsigned)endpoint->port));
    return MCE_OK;
}
