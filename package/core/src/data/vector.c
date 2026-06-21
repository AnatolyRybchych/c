#include <mc/data/vector.h>

extern inline MC_Error __vector_reserve_bytes(MC_Alloc *alloc, void **vector, size_t size);
extern inline MC_Error __vector_push_bytes(MC_Alloc *alloc, void **vector, size_t size, const void *data);
extern inline void __vector_erase_bytes(void *vector, size_t idx, size_t size);
