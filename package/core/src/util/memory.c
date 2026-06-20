
#include <mc/util/memory.h>

extern inline uint16_t mc_u16(uint8_t b1, uint8_t b2);
extern inline uint32_t mc_u32(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4);
extern inline uint64_t mc_u64(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5, uint8_t b6, uint8_t b7, uint8_t b8);
extern inline size_t mc_align(size_t to, size_t value);
extern inline size_t mc_field_padding(size_t offset);
