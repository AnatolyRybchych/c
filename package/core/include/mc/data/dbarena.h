#ifndef MC_DATA_DBARENA_H
#define MC_DATA_DBARENA_H

#include <mc/error.h>
#include <mc/data/alloc.h>

#include <stddef.h>

typedef struct MC_DBArena MC_DBArena;

MC_Error mc_dbarena_init(MC_DBArena **arena, MC_Alloc *base);
void mc_dbarena_destroy(MC_DBArena *arena);

MC_Alloc *mc_dbarena_allocator(MC_DBArena *arena);

MC_Error mc_dbarena_alloc(MC_DBArena *arena, size_t size, void **mem);

void mc_dbarena_swap(MC_DBArena *arena);

void mc_dbarena_reset(MC_DBArena *arena);

#endif // MC_DATA_DBARENA_H
