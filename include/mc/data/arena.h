#ifndef MC_DATA_ARENA_H
#define MC_DATA_ARENA_H

#include <mc/error.h>

#include <stddef.h>

typedef struct MC_Arena MC_Arena;

MC_Error mc_arena_init(MC_Arena **arena);
void mc_arena_destroy(MC_Arena *arena);

MC_Error mc_arena_alloc(MC_Arena *arena, size_t size, void **mem);
void mc_arena_reset(MC_Arena *arena);

#endif // MC_DATA_ARENA_H
