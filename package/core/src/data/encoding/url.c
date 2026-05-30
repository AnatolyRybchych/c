#include <mc/data/encoding/url.h>

#include <stdbool.h>
#include <stdio.h>

#define TO_HEX_DIGIT(VAL) ((VAL) < 10 ? (VAL) + '0' : ((VAL) + 'a' - 10))

MC_Str mc_urldecode(MC_Str encoded, size_t bufsz, char buf[], size_t *decoded_size) {
    MC_Str remaining = encoded;
    size_t decoded = 0;

    while(remaining.beg != remaining.end && decoded != bufsz) {
        char ch = *remaining.beg;
        if(ch == '%') {
            if(mc_str_len(remaining) < 3) {
                break;
            }

            MC_Str hex = MC_STR(remaining.beg + 1, remaining.beg + 3);
            uint64_t byte;
            mc_str_hex_toull(hex, &byte);
            buf[decoded++] = byte;
            remaining.beg += 3;
        }
        else{
            buf[decoded++] = ch;
            remaining.beg += 1;
        }
    }

    *decoded_size = decoded;
    return remaining;
}

MC_Str mc_urlencode(MC_Str decoded, size_t bufsz, char buf[], size_t *encoded_size) {
    MC_Str remaining = decoded;
    size_t encoded = 0;

    for(;remaining.beg != remaining.end && encoded != bufsz; remaining.beg++) {
        uint8_t byte = *remaining.beg;

        if(isalnum(byte)) {
            buf[encoded++] = byte;
            continue;
        }

        else if(bufsz - encoded < 3) {
            break;
        }

        buf[encoded++] = '%';
        buf[encoded++] = TO_HEX_DIGIT(byte >> 4);
        buf[encoded++] = TO_HEX_DIGIT(byte & 0xf);
    }

    *encoded_size = encoded;
    return remaining;
}