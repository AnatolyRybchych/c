#ifndef MC_UTIL_MEMORY_H
#define MC_UTIL_MEMORY_H

#include <mc/error.h>

#include <stdalign.h>
#include <stdint.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdarg.h>

inline uint16_t mc_u16(uint8_t b1, uint8_t b2) {
    return ((uint16_t)b1<<8) | b2;
}

inline uint32_t mc_u32(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4) {
    return ((uint32_t)mc_u16(b1, b2) << 16) | mc_u16(b3, b4);
}

inline uint64_t mc_u64(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5, uint8_t b6, uint8_t b7, uint8_t b8) {
    return ((uint64_t)mc_u32(b1, b2, b3, b4) << 32) | mc_u32(b5, b6, b7, b8);
}

inline size_t mc_align(size_t to, size_t value) {
    return (value + to - 1) & ~(to - 1);
}

inline size_t mc_field_padding(size_t offset) {
    return mc_align(alignof(max_align_t), offset) - offset;
}

#endif // MC_UTIL_MEMORY_H
