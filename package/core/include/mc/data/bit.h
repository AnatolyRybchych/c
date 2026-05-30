#ifndef MC_DATA_BIT_H
#define MC_DATA_BIT_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <memory.h>

inline void mc_bit_set(void *dst, size_t bit, bool value){
    uint8_t *u8dst = dst;
    uint8_t *byte = u8dst + (bit >> 3);
    uint8_t bit_in_byte = 0x80 >> (bit & 0x07);

    *byte = value 
        ? (*byte | bit_in_byte)
        : (*byte & ~bit_in_byte);
}

inline void mc_bit_set_range(void *dst, size_t bit_beg, size_t bit_end, bool value){
    if(bit_beg >= bit_end){
        return;
    }

    size_t bit_last = bit_end - 1;

    uint8_t *u8dst = dst;
    uint8_t *byte_first = u8dst + (bit_beg >> 3);
    uint8_t *byte_last = u8dst + (bit_last >> 3);

    uint8_t first_bit_in_first_byte = 0x80 >> (bit_beg & 0x07);
    uint8_t last_bit_in_last_byte = 0x80 >> (bit_last & 0x07);

    *byte_first = value
        ? *byte_first | ((first_bit_in_first_byte << 1) - 1)
        : *byte_first & ~(first_bit_in_first_byte - 1);

    for(uint8_t *cur = byte_first; cur != byte_last; cur++){
        *cur = value ? 0xFF : 0x00;
    }

    *byte_last = value
        ? *byte_last | ~((last_bit_in_last_byte << 1) - 1)
        : *byte_last & (last_bit_in_last_byte - 1);
}

inline bool mc_bit(const void *src, size_t bit){
    const uint8_t *u8src = src;
    const uint8_t byte = *(u8src + (bit >> 3));
    const uint8_t bit_in_byte = 0x80 >> (bit & 0x07);
    return byte & bit_in_byte;
}

#endif // MC_DATA_BIT_H
