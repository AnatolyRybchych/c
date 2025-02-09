#include <mc/net/address.h>
#include <mc/util/error.h>
#include <mc/util/util.h>
#include <mc/util/minmax.h>
#include <mc/util/memory.h>

#include <mc/data/mstream.h>
#include <mc/data/alloc/sarena.h>

#include <memory.h>

#include <string.h>
#include <stdio.h>

static MC_Error ethernet_addr_write(MC_Stream *out, const MC_EthernetAddress *address);
static MC_Error ipv4_write(MC_Stream *out, const MC_IPv4Address *address);
static MC_Error ipv6_write(MC_Stream *out, const MC_IPv6Address *address);

const char *mc_address_type_str(MC_AddressType type){
    const char *result = NULL;
    MC_ENUM_TO_STRING(result, type, MC_ITER_ADDRESS_TYPE, MC_ADDRTYPE_)
    return result;
}

MC_Error mc_address_to_string(const MC_Address *address, size_t bufsz, char *buf, size_t *written){
    char buffer[512];
    MC_Sarena sarena;
    MC_Alloc *alloc = mc_sarena(&sarena, sizeof buffer, buffer);
    MC_Stream *out;
    MC_RETURN_ERROR(mc_mstream(alloc, &out));

    MC_Error error = mc_address_write(out, address);
    size_t out_size = mc_mstream_size(out);
    MC_RETURN_ERROR(mc_set_cursor(out, 0, MC_CURSOR_FROM_BEG));
    MC_RETURN_ERROR(mc_read(out, bufsz, buf, written));

    buf[MIN(bufsz - 1, out_size)] = 0;

    if(out_size >= bufsz) {
        return MCE_TRUNCATED;
    }

    return error;
}

MC_Error mc_address_write(MC_Stream *out, const MC_Address *address){
    switch (address->type){
    case MC_ADDRTYPE_ETHERNET: return ethernet_addr_write(out, &address->ether);
    case MC_ADDRTYPE_IPV4: return ipv4_write(out, &address->ipv4);
    case MC_ADDRTYPE_IPV6: return ipv6_write(out, &address->ipv6);
    default: return MCE_INVALID_INPUT;
    }
}

static MC_Error ethernet_addr_write(MC_Stream *out, const MC_EthernetAddress *address){
    return mc_fmt(out, "%02X:%02X:%02X:%02X:%02X:%02X",
        address->data[0], address->data[1],
        address->data[2], address->data[3],
        address->data[4], address->data[5]);
}

static MC_Error ipv4_write(MC_Stream *out, const MC_IPv4Address *address){
    return mc_fmt(out, "%d.%d.%d.%d",
        (int)address->data[0],
        (int)address->data[1],
        (int)address->data[2],
        (int)address->data[3]);
}

static MC_Error ipv6_write(MC_Stream *out, const MC_IPv6Address *address) {
    static const uint8_t ipv4_mapped_ipv6_prefix[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff
    };

    const uint8_t *d = address->data;

    if(!memcmp(address->data, ipv4_mapped_ipv6_prefix, sizeof ipv4_mapped_ipv6_prefix)) {
        MC_RETURN_ERROR(mc_fmt(out, "::ffff:"));

        return ipv4_write(out, &(MC_IPv4Address){
            .data = {
                address->data[12],
                address->data[13],
                address->data[14],
                address->data[15]
            }
        });
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
        return mc_fmt(out, "%x:%x:%x:%x:%x:%x:%x:%x",
            MC_U16(d[0], d[1]), MC_U16(d[2], d[3]), MC_U16(d[4], d[5]), \
            MC_U16(d[6], d[7]), MC_U16(d[8], d[9]), MC_U16(d[10], d[11]), \
            MC_U16(d[12], d[13]), MC_U16(d[14], d[15]));
    }

    for(const uint8_t *cur = d; cur != leap_beg; cur += 2){
        MC_RETURN_ERROR(mc_fmt(out, "%x:", MC_U16(cur[0], cur[1])));
    }

    if(d == leap_beg) {
        MC_RETURN_ERROR(mc_fmt(out, ":"));
    }

    for(const uint8_t *cur = leap_end; cur != address->data + sizeof address->data; cur += 2){
        MC_RETURN_ERROR(mc_fmt(out, ":%x", MC_U16(cur[0], cur[1])));
    }

    if(leap_end == address->data + sizeof address->data) {
        MC_RETURN_ERROR(mc_fmt(out, ":"));
    }

    return MCE_OK;
}
