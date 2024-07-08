#include <mc/data/vector.h>

extern inline void *__vector_push_bytes(void *vector, size_t size, const void *data);
extern inline void __vector_erase_bytes(void *vector, size_t idx, size_t size);
