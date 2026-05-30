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

MC_Error mc_address_parse(MC_Address *address, MC_Str str, MC_Str *out_match) {
    MC_Str match, oct1, oct2, oct3, oct4;
    bool ipv4_matches = mc_str_match(str, "^(%d+)%.(%d+)%.(%d+)%.(%d+)",
        &match, &oct1, &oct2, &oct3, &oct4);

    bool ipv6_mapped_ipv4_matches = !ipv4_matches 
        && mc_str_match(str, "^::[fF][fF][fF][fF]:(%d+)%.(%d+)%.(%d+)%.(%d+)",
        &match, &oct1, &oct2, &oct3, &oct4);

    if(ipv4_matches || ipv6_mapped_ipv4_matches) {
        uint64_t octets[4];
        mc_str_toull(oct1, &octets[0]);
        mc_str_toull(oct2, &octets[1]);
        mc_str_toull(oct3, &octets[2]);
        mc_str_toull(oct4, &octets[3]);

        MC_RETURN_INVALID(octets[0] & ~(uint64_t)0xFF);
        MC_RETURN_INVALID(octets[1] & ~(uint64_t)0xFF);
        MC_RETURN_INVALID(octets[2] & ~(uint64_t)0xFF);
        MC_RETURN_INVALID(octets[3] & ~(uint64_t)0xFF);

        if (ipv4_matches) {
            *address = (MC_Address) {
                .type = MC_ADDRTYPE_IPV4,
                .ipv4.data = {octets[0], octets[1], octets[2], octets[3]}
            };
        }
        else {
            *address = (MC_Address) {
                .type = MC_ADDRTYPE_IPV6,
                .ipv6.data = {0, 0, 0, 0, 0, 0, 0, 0, 0 ,0, 0xff, 0xff, octets[0], octets[1], octets[2], octets[3]}
            };
        }
        MC_OPTIONAL_SET(out_match, match);
        return MCE_OK;
    }

    size_t chunks_cnt = 0;
    MC_Str *leap = NULL;
    MC_Str chunks[8];
    for(MC_Str cur = str; chunks_cnt != 8;) {
        MC_Str chunk_match, chunk, delim;
        if(!mc_str_match(cur, "^(%x*)(:*)", &chunk_match, &chunk, &delim)){
            break;
        }

        if(chunks_cnt && mc_str_empty(chunk)) {
            break;
        }

        if(mc_str_len(chunk) > 4) {
            chunks[chunks_cnt++] = MC_STR(chunk.beg, chunk.beg + 4);
            break;
        }

        chunks[chunks_cnt++] = chunk;
        if (mc_str_len(delim) == 2) {
            leap = chunks + chunks_cnt;
        }
        cur.beg = chunk_match.end;
    }

    if(chunks_cnt == 8 || leap) {
        if(!leap) leap = &chunks[chunks_cnt];

        *address = (MC_Address) { .type = MC_ADDRTYPE_IPV6 };
        uint8_t *addr_byte = address->ipv6.data;

        for(MC_Str *chunk = chunks; chunk != leap; chunk++) {
            uint64_t val;
            mc_str_hex_toull(*chunk, &val);
            *addr_byte++ = val >> 8;
            *addr_byte++ = val & 0xff;
        }

        addr_byte = &address->ipv6.data[sizeof address->ipv6.data - 1];
        for(MC_Str *chunk = &chunks[chunks_cnt - 1]; chunk >= leap; chunk--) {
            uint64_t val;
            mc_str_hex_toull(*chunk, &val);
            *addr_byte-- = val & 0xff;
            *addr_byte-- = val >> 8;
        }

        MC_OPTIONAL_SET(out_match, match);
        return MCE_OK;
    }

    if(chunks_cnt == 6 && !leap) {
        *address = (MC_Address) { .type = MC_ADDRTYPE_ETHERNET };
        for(int chunk = 0; chunk != 6; chunk++) {
            MC_RETURN_INVALID(mc_str_len(chunks[chunk]) > 2);

            uint64_t val;
            mc_str_hex_toull(chunks[chunk], &val);
            address->ether.data[chunk] = val;
        }

        MC_OPTIONAL_SET(out_match, match);
        return MCE_OK;
    }

    return MCE_INVALID_INPUT;
}

MC_Error mc_address_parsec(MC_Address *address, const char *str, MC_Str *match) {
    return mc_address_parse(address, MC_STR(str, str + strlen(str)), match);
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
