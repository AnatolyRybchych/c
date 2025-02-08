#include <mc/net/address.h>
#include <mc/util/error.h>
#include <mc/util/util.h>
#include <mc/util/minmax.h>

#include <memory.h>

#include <string.h>
#include <stdio.h>

const char *mc_address_type_str(MC_AddressType type){
    const char *result = NULL;
    MC_ENUM_TO_STRING(result, type, MC_ITER_ADDRESS_TYPE, MC_ADDRTYPE_)
    return result;
}

MC_Error mc_ethernet_addr_to_string(const MC_EthernetAddress *address, size_t bufsz, char *buf, size_t *written){
    size_t off = snprintf(buf, bufsz, "%02X:%02X:%02X:%02X:%02X:%02X",
        address->data[0], address->data[1],
        address->data[2], address->data[3],
        address->data[4], address->data[5]);

    MC_OPTIONAL_SET(written, MIN(off, bufsz));

    return bufsz < off ? MCE_TRUNCATED : MCE_OK;
}

MC_Error mc_ipv4_to_string(const MC_IPv4Address *address, size_t bufsz, char *buf, size_t *written){
    size_t off = snprintf(buf, bufsz, "%d.%d.%d.%d",
        (int)address->data[0],
        (int)address->data[1],
        (int)address->data[2],
        (int)address->data[3]);

    MC_OPTIONAL_SET(written, MIN(off, bufsz));

    return bufsz < off ? MCE_TRUNCATED : MCE_OK;
}

MC_Error mc_ipv6_to_string(const MC_IPv6Address *address, size_t bufsz, char *buf, size_t *written) {
    static const uint8_t ipv4_mapped_ipv6_prefix[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff
    };

    const uint8_t *d = address->data;

    if(!memcmp(address->data, ipv4_mapped_ipv6_prefix, sizeof ipv4_mapped_ipv6_prefix)) {
        size_t off = snprintf(buf, bufsz, "::ffff:");
        if(off > bufsz) {
            MC_OPTIONAL_SET(written, bufsz);
            return MCE_TRUNCATED;
        }
        
        MC_Error error = mc_ipv4_to_string(&(MC_IPv4Address){
            .data = {
                address->data[12],
                address->data[13],
                address->data[14],
                address->data[15]
            }
        }, bufsz - off, buf + off, written);

        if(written) {
            *written += off;
        }

        return error;
    }

    const uint8_t *leap_beg = address->data;
    const uint8_t *leap_end = address->data;

    const uint8_t *cur_leap_beg = leap_beg;
    const uint8_t *cur_leap_end = leap_end;

    for(unsigned i = 0; i < 8; i++) {
        const uint8_t *cur = &address->data[i*2];
        if(cur[0] == 0 && cur[1] == 0) {
            if(cur_leap_end != cur) {
                cur_leap_beg = cur;
            }
            cur_leap_end = cur + 2;

            if(leap_end - leap_beg < cur_leap_end - cur_leap_beg) {
                leap_beg = cur_leap_beg;
                leap_end = cur_leap_end;
            }
        }
    }

    if(leap_beg == leap_end) {
        size_t off = snprintf(buf, bufsz, "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x",
            d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]);

        MC_OPTIONAL_SET(written, MIN(off, bufsz));
        return bufsz < off ? MCE_TRUNCATED : MCE_OK;
    }

    size_t off = 0;
    for(const uint8_t *cur = d; cur != leap_beg; cur += 2){
        off += snprintf(buf + off, MAX(bufsz - off, 0), "%02x%02x:", cur[0], cur[1]);
    }

    if(d == leap_beg) {
        off += snprintf(buf + off, MAX(bufsz - off, 0), ":");
    }

    for(const uint8_t *cur = leap_end; cur != address->data + sizeof address->data; cur += 2){
        off += snprintf(buf + off, MAX(bufsz - off, 0), ":%02x%02x", cur[0], cur[1]);
    }

    if(leap_end == address->data + sizeof address->data) {
        off += snprintf(buf + off, MAX(bufsz - off, 0), ":");
    }

    MC_OPTIONAL_SET(written, MIN(off, bufsz));
    return bufsz < off ? MCE_TRUNCATED : MCE_OK;
}

MC_Error mc_address_to_string(const MC_Address *address, size_t bufsz, char *buf, size_t *written){
    switch (address->type){
    case MC_ADDRTYPE_ETHERNET: return mc_ethernet_addr_to_string(&address->ether, bufsz, buf, written);
    case MC_ADDRTYPE_IPV4: return mc_ipv4_to_string(&address->ipv4, bufsz, buf, written);
    case MC_ADDRTYPE_IPV6: return mc_ipv6_to_string(&address->ipv6, bufsz, buf, written);
    default: return MCE_INVALID_INPUT;
    }
}
