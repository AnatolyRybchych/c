#include <mc/data/arena.h>

#include <mc/util/memory.h>
#include <mc/util/minmax.h>

#include <malloc.h>
#include <stdalign.h>

#define INITIAL_CAPACITY 1024

struct MC_Arena{
    struct Buffer{
        struct Buffer *next;
        uint8_t *capacity_end;
        uint8_t *available;
        alignas(void*) uint8_t beg[];
    } *buffer;
};

MC_Error mc_arena_init(MC_Arena **ret_arena){
    MC_Arena *arena;
    struct Buffer *buffer;

    MC_RETURN_ERROR(mc_malloc_all(
        &arena, sizeof(MC_Arena),
        &buffer, sizeof(struct Buffer) + INITIAL_CAPACITY,
        NULL));
    
    buffer->available = buffer->beg;
    buffer->capacity_end = buffer->beg + INITIAL_CAPACITY;
    buffer->next = NULL;

    arena->buffer = buffer;

    *ret_arena = arena;
    return MCE_OK;
}

void mc_arena_destroy(MC_Arena *arena){
    mc_arena_reset(arena);
    free(arena->buffer);
    free(arena);
}

MC_Error mc_arena_alloc(MC_Arena *arena, size_t size, void **mem){
    size = MC_ALIGN(alignof(void*), size);

    if(arena->buffer->available + size > arena->buffer->capacity_end){
        size_t capacity = arena->buffer->capacity_end - arena->buffer->beg;
        size_t exp_capacity = capacity * 2 + 1;
        size_t new_capacity = MAX(exp_capacity, size);
        struct Buffer *buffer = malloc(sizeof(struct Buffer) + new_capacity);
        if(buffer == NULL){
            return MCE_OUT_OF_MEMORY;
        }

        buffer->capacity_end = buffer->beg + new_capacity;
        buffer->available = buffer->beg;
        buffer->next = arena->buffer;
        arena->buffer = buffer;
    }

    *mem = arena->buffer->available;
    arena->buffer->available += size;
    return MCE_OK;
}

void mc_arena_reset(MC_Arena *arena){
    for(struct Buffer *b = arena->buffer->next, *tmp; b;){
        tmp = b->next;
        free(b);
        b = tmp;
    }

    arena->buffer->next = NULL;
    arena->buffer->available = arena->buffer->beg;
}