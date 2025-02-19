#include <mc/net/endpoint.h>
#include <mc/data/mstream.h>
#include <mc/data/alloc/sarena.h>
#include <mc/util/error.h>
#include <mc/util/minmax.h>
#include <mc/util/util.h>

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

MC_Error mc_endpoint_parse(MC_Endpoint *endpoint, MC_Str str, MC_Str *res_match) {
    *endpoint = (MC_Endpoint){0};

    MC_Str match, proto, urn;
    if(mc_str_match(str, "^(%w+)://(.*)", &match, &proto, &urn)) {
        MC_Address address;
        MC_Str port_str = {0};
        MC_Str enplaced_addr;
        MC_Str dummy;
        if(mc_str_match(urn, "^%[([0-9a-fA-F:]+)%]:?(%d*)", &dummy, &enplaced_addr, &port_str)) {
            MC_RETURN_INVALID(mc_address_parse(&address, enplaced_addr, NULL));
        }
        else {
            MC_Str address_match;
            MC_RETURN_INVALID(mc_address_parse(&address, urn, &address_match));
            MC_RETURN_INVALID(address.type != MC_ADDRTYPE_IPV4);
            MC_Str port_src = MC_STR(address_match.end, urn.end);
            mc_str_match(port_src, "^:(%d+)", &dummy, &port_str);
        }

        uint64_t port = 0;
        mc_str_toull(port_str, &port);
        MC_RETURN_INVALID(port >> 16);

        if(mc_str_equ(proto, MC_STRC("tcp"))) {
            *endpoint = (MC_Endpoint) {
                .type = MC_ENDPOINT_TCP,
                .tcp = {
                    .port = port,
                    .address = address
                }
            };
            MC_OPTIONAL_SET(res_match, match);
            return MCE_OK;
        }

        if(mc_str_equ(proto, MC_STRC("udp"))) {
            *endpoint = (MC_Endpoint) {
                .type = MC_ENDPOINT_UDP,
                .tcp = {
                    .port = port,
                    .address = address
                }
            };
            MC_OPTIONAL_SET(res_match, match);
            return MCE_OK;
        }

        return MCE_INVALID_INPUT;
    }

    MC_Address address;
    MC_RETURN_ERROR(mc_address_parse(&address, str, &match));

    switch (address.type){
    case MC_ADDRTYPE_ETHERNET:
        *endpoint = (MC_Endpoint) {
            .type = MC_ENDPOINT_ETHERNET,
            .ether = address.ether
        };
        MC_OPTIONAL_SET(res_match, match);
        return MCE_OK;
    case MC_ADDRTYPE_IPV4:
        *endpoint = (MC_Endpoint) {
            .type = MC_ENDPOINT_IPV4,
            .ipv4 = address.ipv4
        };
        MC_OPTIONAL_SET(res_match, match);
        return MCE_OK;
    case MC_ADDRTYPE_IPV6:
        *endpoint = (MC_Endpoint) {
            .type = MC_ENDPOINT_IPV6,
            .ipv6 = address.ipv6
        };
        MC_OPTIONAL_SET(res_match, match);
        return MCE_OK;
    default:
        return MCE_INVALID_INPUT;
    }

}

MC_Error mc_endpoint_parsec(MC_Endpoint *endpoint, const char *str, MC_Str *res_match) {
    return mc_endpoint_parse(endpoint, MC_STR(str, str + strlen(str)), res_match);
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
