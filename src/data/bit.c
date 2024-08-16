#include <mc/data/bit.h>

extern inline void mc_bit_set(void *dst, size_t bit, bool value);
extern inline void mc_bit_set_range(void *dst, size_t first_bit, size_t last_bit, bool value);
extern inline bool mc_bit(const void *src, size_t bit);
