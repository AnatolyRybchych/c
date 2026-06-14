#include <mc/data/dbarena.h>

#include <mc/data/arena.h>
#include <mc/util/error.h>

struct MC_DBArena{
    MC_Alloc alloc;
    MC_Alloc *base;
    MC_Arena *arenas[2];
    int front;
};

static void *alloc(MC_Alloc *this, size_t size);

MC_Error mc_dbarena_init(MC_DBArena **ret_arena, MC_Alloc *base){
    MC_DBArena *arena;
    MC_RETURN_ERROR(mc_alloc(base, sizeof(MC_DBArena), (void**)&arena));

    MC_Error status = mc_arena_init(&arena->arenas[0], base);
    if(status != MCE_OK){
        mc_free(base, arena);
        return status;
    }

    status = mc_arena_init(&arena->arenas[1], base);
    if(status != MCE_OK){
        mc_arena_destroy(arena->arenas[0]);
        mc_free(base, arena);
        return status;
    }

    arena->alloc = (MC_Alloc){
        .alloc = alloc,
        .free = NULL
    };
    arena->base = base;
    arena->front = 0;

    *ret_arena = arena;
    return MCE_OK;
}

void mc_dbarena_destroy(MC_DBArena *arena){
    mc_arena_destroy(arena->arenas[0]);
    mc_arena_destroy(arena->arenas[1]);
    mc_free(arena->base, arena);
}

MC_Alloc *mc_dbarena_allocator(MC_DBArena *arena){
    return &arena->alloc;
}

MC_Error mc_dbarena_alloc(MC_DBArena *arena, size_t size, void **mem){
    return mc_arena_alloc(arena->arenas[arena->front], size, mem);
}

void mc_dbarena_swap(MC_DBArena *arena){
    arena->front = !arena->front;
    mc_arena_reset(arena->arenas[arena->front]);
}

void mc_dbarena_reset(MC_DBArena *arena){
    mc_arena_reset(arena->arenas[0]);
    mc_arena_reset(arena->arenas[1]);
}

static void *alloc(MC_Alloc *this, size_t size){
    MC_DBArena *arena = (MC_DBArena*)this;

    void *ptr;
    return mc_dbarena_alloc(arena, size, &ptr) == MCE_OK
        ? ptr : NULL;
}
