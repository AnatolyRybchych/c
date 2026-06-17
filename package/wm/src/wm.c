#include <mc/wm/wm.h>
#include <mc/wm/target.h>
#include <mc/wm/event.h>
#include <mc/wm/resolver.h>

#include <mc/data/string.h>
#include <mc/data/vector.h>
#include <mc/data/arena.h>
#include <mc/data/dbarena.h>

#include <mc/os/file.h>
#include <mc/util/assert.h>
#include <mc/util/memory.h>
#include <mc/util/error.h>

#include <malloc.h>
#include <memory.h>

#define INDICATIONS_BUFFER_SIZE (MC_WM_MAX_INDICATIONS_PER_EVENT * 16)
#define DEFAULT_EVENT_QUEUE_LIMIT 256

#define WM_LOGE(FMT, ...) if(wm->log) mc_fmt(wm->log, "[WM] " FMT "\n", ##__VA_ARGS__)
#define WM_LOG_DEV(FMT, ...) if(wm->log) mc_fmt(wm->log, "%s() [DEV][WM][%s] " FMT "\n", __func__, wm->vtab.name, ##__VA_ARGS__)

#define RETURN_IF_REF_BUSY(REF) \
    do { \
        if(is_ref_busy(REF)){ \
            return MCE_BUSY; \
        } \
    } while(0)

#define RETURN_IF_WM_BUSY(REF) \
    do { \
        if(!wm_of(REF)->is_alive){ \
            return MCE_BUSY; \
        } \
    } while(0)

MC_DEFINE_VECTOR(Windows, MC_WMWindow*);
MC_DEFINE_VECTOR(ForeignWindows, MC_ForeignWindow*);

typedef struct EventGroup{
    MC_WMEventType offset;
    size_t size;
    size_t reserve;
    MC_String *name;
    MC_WMEventDefinition *events;
    MC_Error (*to_json)(MC_Alloc *alloc, const MC_WMEvent *event, MC_Json *json);
    MC_Error (*from_json)(MC_Alloc *alloc, const MC_Json *json, MC_WMEvent *event);
} EventGroup;
MC_DEFINE_VECTOR(EventGroups, EventGroup);

typedef struct EventNode{
    struct EventNode *next;
    MC_WMEvent event;
} EventNode;

typedef struct indicationNode indicationNode;

typedef enum ReferenceType{
    REFERENCE_INTERNAL,
    REFERENCE_FOREIGN,
} ReferenceType;

struct MC_WindowRef{
    ReferenceType type;
    unsigned refcount;
};

struct MC_WMWindow{
    MC_WindowRef ref;
    MC_WM *wm;
    struct MC_TargetWMWindow *target;
    bool is_alive;
    MC_WindowParameters parameters;
    struct{
        MC_Rect2IU rect[MC_WM_AREA_COUNT];
        MC_Vec2i mouse_pos;
        bool mouse_over;
        MC_String *title;
        MC_WMWindowState state;
    } cached;
    MC_Stream *title_stream;
    struct MC_Graphics *g;
    alignas(void*) uint8_t data[];
};

struct MC_ForeignWindow{
    MC_WindowRef ref;
    MC_WM *wm;
    uint64_t identity;
    struct MC_TargetForeignWindow *target;
    alignas(void*) uint8_t data[];
};

struct MC_WMRef{
    unsigned refcount;
};

struct MC_WM{
    MC_WMRef ref;
    bool is_alive;
    MC_WMVtab vtab;
    MC_Stream *log;
    struct MC_TargetWM *target;
    struct MC_TargetWMEvent *target_event;

    MC_WMEvents events;

    unsigned pending_indications;
    MC_TargetIndication indications[INDICATIONS_BUFFER_SIZE];

    Windows *windows;
    ForeignWindows *foreign_windows;

    MC_WMEventSubscription *event_subs;

    MC_DBArena *event_arena;
    MC_Alloc *event_alloc;
    EventNode *event_queue;
    EventNode *event_queue_tail;
    EventNode *event_queue_mark;
    EventNode *pending_user;
    EventNode *pending_user_tail;
    size_t event_count;
    size_t event_limit;

    EventGroups *event_groups;
    MC_WMEventType event_next;

    MC_Arena *arena;

    alignas(void*) uint8_t data[];
};

struct MC_WMEventSubscription{
    MC_WMEventSubscription *next;
    MC_WM *wm;
    MC_WMEventMatch match;
    void (*callback)(MC_WMRef *wm, const MC_WMEvent *event, void *user_data);
    void *user_data;
};

static void dump_vtable(MC_WM *wm);
static void handle_duplicate_indications(MC_WM *wm);
static MC_WMEvent translate_indication(MC_WM *wm);
static void discard_indication(MC_WM *wm);
static MC_WMWindow *window_from_target(MC_WM *wm, struct MC_TargetWMWindow *target);
static MC_WMWindow *find_window(MC_WM *wm, uint64_t identity);
static MC_ForeignWindow *find_foreign(MC_WM *wm, uint64_t identity);
static bool is_ref_busy(MC_WindowRef *ref);
static MC_Error alloc_foreign(MC_WM *wm, MC_ForeignWindow **out);
static MC_Error track_foreign(MC_WM *wm, MC_ForeignWindow *foreign);
static void window_free(MC_WMWindow *window);
static void foreign_free(MC_ForeignWindow *foreign);

static MC_Error owner_close(MC_WMWindow *window);
static MC_Error owner_focus(MC_WMWindow *window);
static MC_Error owner_get_title(MC_WMWindow *window, char *utf8, size_t cap, size_t *len);
static MC_Error owner_set_title(MC_WMWindow *window, MC_Str title);
static MC_Error owner_set_state(MC_WMWindow *window, MC_WMWindowState state);
static MC_Error owner_get_state(MC_WMWindow *window, MC_WMWindowState *state);
static MC_Error owner_set_rect(MC_WMWindow *window, MC_WMArea area, MC_Rect2IU rect);
static MC_Error owner_get_rect(MC_WMWindow *window, MC_WMArea area, MC_Rect2IU *rect);

static MC_Error foreign_close(MC_ForeignWindow *foreign);
static MC_Error foreign_focus(MC_ForeignWindow *foreign);
static MC_Error foreign_set_title(MC_ForeignWindow *foreign, MC_Str title);
static MC_Error foreign_set_state(MC_ForeignWindow *foreign, MC_WMWindowState state);
static MC_Error foreign_get_title(MC_ForeignWindow *foreign, char *utf8, size_t cap, size_t *len);
static MC_Error foreign_get_state(MC_ForeignWindow *foreign, MC_WMWindowState *state);
static MC_Error foreign_set_rect(MC_ForeignWindow *foreign, MC_WMArea area, MC_Rect2IU rect);
static MC_Error foreign_get_rect(MC_ForeignWindow *foreign, MC_WMArea area, MC_Rect2IU *rect);
static MC_Error foreign_is_system(MC_ForeignWindow *foreign, bool *out);

static const MC_WMEvents indication_category[MC_WMIND_COUNT] = {
    #define MC_WMIDN(NAME, DUP_ACTION, CATEGORY) [MC_WMIND_##NAME] = MC_WM_EVENTS_##CATEGORY,
    MC_ITER_INDICATIONS()
    #undef MC_WMIDN
};

MC_Error mc_wm_init(MC_WM **ret_wm, const MC_WMVtab *vtab){
    if(!vtab || !vtab->init){
        return MCE_NOT_SUPPORTED;
    }

    MC_WM *wm;
    struct MC_TargetWMEvent *target_event;

    MC_Error allocation_status = mc_alloc_all(NULL,
        &wm, sizeof(MC_WM) + vtab->wm_size,
        &target_event, vtab->event_size,
        NULL);

    if(allocation_status != MCE_OK){
        return allocation_status;
    }

    memset(wm, 0, sizeof(MC_WM) + vtab->wm_size);
    wm->target_event = target_event;

    memcpy(&wm->vtab, vtab, sizeof(MC_WMVtab));
    wm->target = (struct MC_TargetWM*)wm->data;
    wm->log = MC_STDERR;
    wm->events = MC_WM_EVENTS_CORE | MC_WM_EVENTS_WINDOW_CORE;

    wm->event_limit = DEFAULT_EVENT_QUEUE_LIMIT;

    MC_Error event_arena_status = mc_dbarena_init(&wm->event_arena, NULL);
    if(event_arena_status != MCE_OK){
        mc_free(NULL, wm->target_event);
        mc_free(NULL, wm);
        return event_arena_status;
    }
    wm->event_alloc = mc_dbarena_allocator(wm->event_arena);

    MC_Error arena_status = mc_arena_init(&wm->arena, NULL);
    if(arena_status != MCE_OK){
        mc_dbarena_destroy(wm->event_arena);
        mc_free(NULL, wm->target_event);
        mc_free(NULL, wm);
        return arena_status;
    }

    MC_Error init_status = vtab->init(wm->target, wm->log, mc_arena_allocator(wm->arena));
    if(init_status != MCE_OK){
        mc_arena_destroy(wm->arena);
        mc_dbarena_destroy(wm->event_arena);
        mc_free(NULL, wm->target_event);
        mc_free(NULL, wm);
        return init_status;
    }

    extern const MC_WMEventGroupDef *mc_wm_builtin_group_def(void);
    MC_WMEventType builtin_offset;
    MC_Error group_status = mc_wm_register_event_group(&wm->ref, mc_wm_builtin_group_def(), &builtin_offset);
    if(group_status != MCE_OK){
        if(wm->vtab.destroy){
            wm->vtab.destroy(wm->target);
        }
        mc_arena_destroy(wm->arena);
        mc_dbarena_destroy(wm->event_arena);
        mc_free(NULL, wm->target_event);
        mc_free(NULL, wm);
        return group_status;
    }

    wm->ref.refcount = 1;
    wm->is_alive = true;

    mc_wm_resolver_register(vtab);
    mc_wm_resolver_set(wm);

    *ret_wm = wm;
    (void)dump_vtable;
    // dump_vtable(wm);

    return MCE_OK;
}

static MC_WM *wm_of(MC_WMRef *ref){
    return (MC_WM*)ref;
}

MC_WMRef *mc_wm_get_ref(MC_WM *wm){
    return &wm->ref;
}

MC_WMRef *mc_wm_ref(MC_WMRef *ref){
    wm_of(ref)->ref.refcount++;

    return ref;
}

static void wm_teardown(MC_WM *wm){
    if(!wm->is_alive){
        return;
    }
    wm->is_alive = false;

    mc_wm_resolver_forget(wm);

    while(MC_VECTOR_SIZE(wm->windows) > 0){
        MC_WMWindow *window = wm->windows->beg[0];
        if(window->is_alive && wm->vtab.destroy_window){
            wm->vtab.destroy_window(wm->target, window->target);
        }

        MC_VECTOR_ERASE(wm->windows, 0, 1);
        mc_free(NULL, window);
    }

    while(MC_VECTOR_SIZE(wm->foreign_windows) > 0){
        MC_ForeignWindow *foreign = wm->foreign_windows->beg[0];
        if(wm->vtab.destroy_foreign_window){
            wm->vtab.destroy_foreign_window(wm->target, foreign->target);
        }

        MC_VECTOR_ERASE(wm->foreign_windows, 0, 1);
        mc_free(NULL, foreign);
    }

    while(wm->event_subs != NULL){
        MC_WMEventSubscription *next = wm->event_subs->next;
        mc_free(NULL, wm->event_subs);
        wm->event_subs = next;
    }

    if(wm->event_groups != NULL){
        EventGroup *g;
        MC_VECTOR_EACH(wm->event_groups, g){
            for(size_t i = 0; i < g->size; i++){
                mc_free(NULL, (void*)g->events[i].name);
            }
            mc_free(NULL, g->events);
            mc_free(NULL, g->name);
        }
        MC_VECTOR_FREE(wm->event_groups);
        wm->event_groups = NULL;
    }

    if(wm->vtab.destroy){
        wm->vtab.destroy(wm->target);
    }

    MC_VECTOR_FREE(wm->windows);
    MC_VECTOR_FREE(wm->foreign_windows);

    mc_arena_destroy(wm->arena);
    mc_dbarena_destroy(wm->event_arena);
}

static void wm_release(MC_WM *wm){
    MC_ASSERT_FAULT(wm->ref.refcount > 0);

    wm->ref.refcount--;
    if(wm->ref.refcount > 0){
        return;
    }

    wm_teardown(wm);

    mc_free(NULL, wm->target_event);
    mc_free(NULL, wm);
}

void mc_wm_destroy(MC_WM *wm){
    wm_teardown(wm);
    wm_release(wm);
}

void mc_wm_unref(MC_WMRef *ref){
    wm_release(wm_of(ref));
}

const char *mc_wm_impl_name(MC_WMRef *ref){
    return wm_of(ref)->vtab.name;
}

struct MC_TargetWM *mc_wm_get_target(MC_WMRef *ref){
    MC_WM *wm = wm_of(ref);
    return wm->is_alive ? wm->target : NULL;
}

MC_Error mc_wm_request_events(MC_WMRef *ref, MC_WMEvents events){
    RETURN_IF_WM_BUSY(ref);

    MC_WM *wm = wm_of(ref);
    wm->events |= events;

    if(wm->vtab.request_events){
        return wm->vtab.request_events(wm->target, events);
    }

    return MCE_OK;
}

MC_WMEvents mc_wm_get_requested_events(MC_WMRef *ref){
    return wm_of(ref)->events;
}

MC_Error mc_wm_window_init(MC_WMRef *ref, MC_WMWindow **ret_window){
    RETURN_IF_WM_BUSY(ref);

    MC_WM *wm = wm_of(ref);
    *ret_window = NULL;
    MC_WMVtab *v = &wm->vtab;

    if(v->init_window == NULL){
        return MCE_NOT_SUPPORTED;
    }

    MC_WMWindow *window;
    MC_RETURN_ERROR(mc_alloc(NULL, sizeof(MC_WMWindow) + v->window_size, (void**)&window));

    memset(window, 0, sizeof(MC_WMWindow) + v->window_size);
    window->ref.type = REFERENCE_INTERNAL;
    window->ref.refcount = 1;
    window->wm = wm;
    window->target = (struct MC_TargetWMWindow*)window->data;
    window->is_alive = true;

    Windows *new_windows = MC_VECTOR_PUSHN(wm->windows, 1, &window);
    if(new_windows == NULL){
        mc_free(NULL, window);
        return MCE_OUT_OF_MEMORY;
    }
    wm->windows = new_windows;

    MC_Error status = v->init_window(wm->target, window->target, &window->parameters);
    if(status != MCE_OK){
        wm->windows->end--;
        mc_free(NULL, window);
        return MCE_OUT_OF_MEMORY;
    }

    *ret_window = window;
    return MCE_OK;
}

void mc_wm_window_destroy(MC_WMWindow *window){
    MC_WM *wm = window->wm;
    MC_WMVtab *v = &wm->vtab;

    MC_ASSERT_FAULT(window->ref.refcount > 0);

    if(window->is_alive){
        if(v->destroy_window){
            v->destroy_window(wm->target, window->target);
        }

        window->is_alive = false;
    }

    window->ref.refcount--;
    if(window->ref.refcount == 0){
        window_free(window);
    }
}

struct MC_TargetWMWindow *mc_wm_window_get_target(MC_WMWindow *window){
    return window->target;
}

MC_WindowRef *mc_wm_window_get_ref(MC_WMWindow *window){
    return &window->ref;
}

MC_Error mc_wm_window_get_graphic(MC_WMWindow *window, struct MC_Graphics **g){
    MC_WM *wm = window->wm;
    MC_WMVtab *v = &wm->vtab;

    if(window->g){
        *g = window->g;
        return MCE_OK;
    }

    if(!v->create_window_graphic){
        *g = NULL;
        return MCE_NOT_SUPPORTED;
    }

    MC_Error status = v->create_window_graphic(wm->target, window->target, &window->g);
    *g = status == MCE_OK ? window->g : NULL;

    return status;
}

MC_Str mc_wm_window_cached_get_title(MC_WMWindow *window){
    return mc_string_str(window->cached.title);
}

MC_Rect2IU mc_wm_window_cached_get_rect(MC_WMWindow *window, MC_WMArea area){
    return window->cached.rect[area];
}

MC_Vec2i mc_wm_window_cached_get_position(MC_WMWindow *window, MC_WMArea area){
    return (MC_Vec2i){
        .x = window->cached.rect[area].x,
        .y = window->cached.rect[area].y
    };
}

MC_Size2U mc_wm_window_cached_get_size(MC_WMWindow *window, MC_WMArea area){
    return (MC_Size2U){
        .width = window->cached.rect[area].width,
        .height = window->cached.rect[area].height
    };
}

MC_WMWindowState mc_wm_window_cached_get_state(MC_WMWindow *window){
    return window->cached.state;
}

bool mc_wm_window_cached_is_mouse_over(MC_WMWindow *window){
    return window->cached.mouse_over;
}

static MC_Error window_from_identity(MC_WM *wm, uint64_t identity, MC_WindowRef **ret_window){
    MC_WMVtab *v = &wm->vtab;

    MC_WMWindow *local = find_window(wm, identity);
    if(local){
        local->ref.refcount++;
        *ret_window = &local->ref;
        return MCE_OK;
    }

    MC_ForeignWindow *existing = find_foreign(wm, identity);
    if(existing){
        existing->ref.refcount++;
        *ret_window = &existing->ref;
        return MCE_OK;
    }

    if(v->resolve_temporary_identity == NULL){
        return MCE_NOT_FOUND;
    }

    MC_TargetResolvedIdentity resolved;
    MC_RETURN_ERROR(v->resolve_temporary_identity(wm->target, identity, &resolved));

    if(resolved.type == MC_TARGET_IDENTITY_WINDOW){
        MC_WMWindow *window = window_from_target(wm, resolved.as.window);
        if(window == NULL){
            return MCE_NOT_FOUND;
        }

        window->ref.refcount++;
        *ret_window = &window->ref;
        return MCE_OK;
    }

    MC_ForeignWindow *foreign;
    MC_RETURN_ERROR(alloc_foreign(wm, &foreign));
    foreign->identity = identity;
    memcpy(foreign->target, resolved.as.foreign_window, v->foreign_window_size);

    if(track_foreign(wm, foreign) != MCE_OK){
        mc_free(NULL, foreign);
        return MCE_OUT_OF_MEMORY;
    }

    foreign->ref.refcount = 1;
    *ret_window = &foreign->ref;
    return MCE_OK;
}

MC_Error mc_wm_get_focused_window(MC_WMRef *ref, MC_WindowRef **ret_window){
    RETURN_IF_WM_BUSY(ref);

    MC_WM *wm = wm_of(ref);
    *ret_window = NULL;
    MC_WMVtab *v = &wm->vtab;

    if(v->get_focused_window == NULL || v->resolve_temporary_identity == NULL){
        return MCE_NOT_SUPPORTED;
    }

    uint64_t identity;
    MC_RETURN_ERROR(v->get_focused_window(wm->target, &identity));

    return window_from_identity(wm, identity, ret_window);
}

MC_Error mc_wm_get_hovered_window(MC_WMRef *ref, MC_WindowRef **ret_window){
    RETURN_IF_WM_BUSY(ref);

    MC_WM *wm = wm_of(ref);
    *ret_window = NULL;
    MC_WMVtab *v = &wm->vtab;

    if(v->get_hovered_window == NULL || v->resolve_temporary_identity == NULL){
        return MCE_NOT_SUPPORTED;
    }

    uint64_t identity;
    MC_RETURN_ERROR(v->get_hovered_window(wm->target, &identity));

    return window_from_identity(wm, identity, ret_window);
}

MC_Error mc_wm_get_all_windows(MC_WMRef *ref, MC_Error (*visit)(MC_WindowRef *window, void *ctx), void *ctx){
    RETURN_IF_WM_BUSY(ref);

    MC_WM *wm = wm_of(ref);
    MC_WMVtab *v = &wm->vtab;

    if(v->get_all_windows == NULL || v->resolve_temporary_identity == NULL){
        return MCE_NOT_SUPPORTED;
    }

    if(visit == NULL){
        return MCE_INVALID_INPUT;
    }

    uint64_t *identities;
    size_t count;
    MC_RETURN_ERROR(v->get_all_windows(wm->target, &identities, &count));

    for(size_t i = 0; i < count; i++){
        MC_WindowRef *window;
        if(window_from_identity(wm, identities[i], &window) != MCE_OK){
            continue;
        }

        MC_RETURN_ERROR(visit(window, ctx));
    }

    return MCE_OK;
}

MC_Error mc_wm_resolve_window(MC_WMRef *ref, uint64_t identity, MC_WindowRef **window){
    if(ref == NULL || window == NULL){
        return MCE_INVALID_INPUT;
    }

    RETURN_IF_WM_BUSY(ref);

    MC_WM *wm = wm_of(ref);
    *window = NULL;

    return window_from_identity(wm, identity, window);
}

struct MC_TargetForeignWindow *mc_wm_window_get_foreign_target(MC_WindowRef *window){
    if(window->type == REFERENCE_FOREIGN){
        return ((MC_ForeignWindow*)window)->target;
    }

    return NULL;
}

MC_Error mc_wm_window_get_identity(MC_WindowRef *window, uint64_t *out){
    if(window == NULL || out == NULL){
        return MCE_INVALID_INPUT;
    }

    RETURN_IF_REF_BUSY(window);

    switch(window->type){
    case REFERENCE_FOREIGN: *out = ((MC_ForeignWindow*)window)->identity; return MCE_OK;
    case REFERENCE_INTERNAL: *out = ((MC_WMWindow*)window)->parameters.identity; return MCE_OK;
    default: return MCE_NOT_SUPPORTED;
    }
}

MC_WindowRef *mc_wm_window_ref(MC_WindowRef *window){
    window->refcount++;
    return window;
}

void mc_wm_window_unref(MC_WindowRef *window){
    if(window == NULL){
        return;
    }

    if(window->refcount > 0){
        window->refcount--;
    }

    if(window->refcount > 0){
        return;
    }

    if(window->type == REFERENCE_INTERNAL){
        window_free((MC_WMWindow*)window);
    }
    else{
        foreign_free((MC_ForeignWindow*)window);
    }
}

unsigned mc_wm_window_refcount(MC_WindowRef *window){
    return window->refcount;
}

bool mc_wm_window_is_alive(MC_WindowRef *window){
    if(window->type == REFERENCE_INTERNAL){
        return ((MC_WMWindow*)window)->is_alive;
    }

    return true;
}

MC_Error mc_wm_window_set_title(MC_WindowRef *window, MC_Str title){
    RETURN_IF_REF_BUSY(window);

    switch(window->type){
    case REFERENCE_FOREIGN: return foreign_set_title((MC_ForeignWindow*)window, title);
    case REFERENCE_INTERNAL: return owner_set_title((MC_WMWindow*)window, title);
    default: return MCE_NOT_SUPPORTED;
    }
}

MC_Error mc_wm_window_set_rect(MC_WindowRef *window, MC_WMArea area, MC_Rect2IU rect){
    RETURN_IF_REF_BUSY(window);

    switch(window->type){
    case REFERENCE_FOREIGN: return foreign_set_rect((MC_ForeignWindow*)window, area, rect);
    case REFERENCE_INTERNAL: return owner_set_rect((MC_WMWindow*)window, area, rect);
    default: return MCE_NOT_SUPPORTED;
    }
}

MC_Error mc_wm_window_set_size(MC_WindowRef *window, MC_WMArea area, MC_Size2U size){
    MC_Rect2IU rect;
    MC_RETURN_ERROR(mc_wm_window_get_rect(window, area, &rect));

    rect.width = size.width;
    rect.height = size.height;
    return mc_wm_window_set_rect(window, area, rect);
}

MC_Error mc_wm_window_set_position(MC_WindowRef *window, MC_WMArea area, MC_Vec2i position){
    MC_Rect2IU rect;
    MC_RETURN_ERROR(mc_wm_window_get_rect(window, area, &rect));

    rect.x = position.x;
    rect.y = position.y;
    return mc_wm_window_set_rect(window, area, rect);
}

MC_Error mc_wm_window_close(MC_WindowRef *window){
    RETURN_IF_REF_BUSY(window);

    switch(window->type){
    case REFERENCE_FOREIGN: return foreign_close((MC_ForeignWindow*)window);
    case REFERENCE_INTERNAL: return owner_close((MC_WMWindow*)window);
    default: return MCE_NOT_SUPPORTED;
    }
}

MC_Error mc_wm_window_focus(MC_WindowRef *window){
    RETURN_IF_REF_BUSY(window);

    switch(window->type){
    case REFERENCE_FOREIGN: return foreign_focus((MC_ForeignWindow*)window);
    case REFERENCE_INTERNAL: return owner_focus((MC_WMWindow*)window);
    default: return MCE_NOT_SUPPORTED;
    }
}

MC_Error mc_wm_window_set_state(MC_WindowRef *window, MC_WMWindowState state){
    RETURN_IF_REF_BUSY(window);

    switch(window->type){
    case REFERENCE_FOREIGN: return foreign_set_state((MC_ForeignWindow*)window, state);
    case REFERENCE_INTERNAL: return owner_set_state((MC_WMWindow*)window, state);
    default: return MCE_NOT_SUPPORTED;
    }
}

MC_Error mc_wm_window_get_title(MC_WindowRef *window, char *utf8, size_t cap, size_t *len){
    RETURN_IF_REF_BUSY(window);

    switch(window->type){
    case REFERENCE_FOREIGN: return foreign_get_title((MC_ForeignWindow*)window, utf8, cap, len);
    case REFERENCE_INTERNAL: return owner_get_title((MC_WMWindow*)window, utf8, cap, len);
    default: return MCE_NOT_SUPPORTED;
    }
}

MC_Error mc_wm_window_get_rect(MC_WindowRef *window, MC_WMArea area, MC_Rect2IU *rect){
    RETURN_IF_REF_BUSY(window);

    switch(window->type){
    case REFERENCE_FOREIGN: return foreign_get_rect((MC_ForeignWindow*)window, area, rect);
    case REFERENCE_INTERNAL: return owner_get_rect((MC_WMWindow*)window, area, rect);
    default: return MCE_NOT_SUPPORTED;
    }
}

MC_Error mc_wm_window_get_size(MC_WindowRef *window, MC_WMArea area, MC_Size2U *size){
    MC_Rect2IU rect;
    MC_RETURN_ERROR(mc_wm_window_get_rect(window, area, &rect));

    *size = (MC_Size2U){.width = rect.width, .height = rect.height};
    return MCE_OK;
}

MC_Error mc_wm_window_get_position(MC_WindowRef *window, MC_WMArea area, MC_Vec2i *position){
    MC_Rect2IU rect;
    MC_RETURN_ERROR(mc_wm_window_get_rect(window, area, &rect));

    *position = (MC_Vec2i){.x = rect.x, .y = rect.y};
    return MCE_OK;
}

MC_Error mc_wm_window_get_state(MC_WindowRef *window, MC_WMWindowState *state){
    RETURN_IF_REF_BUSY(window);

    switch(window->type){
    case REFERENCE_FOREIGN: return foreign_get_state((MC_ForeignWindow*)window, state);
    case REFERENCE_INTERNAL: return owner_get_state((MC_WMWindow*)window, state);
    default: return MCE_NOT_SUPPORTED;
    }
}

MC_Error mc_wm_window_is_system(MC_WindowRef *window, bool *out){
    if(window == NULL || out == NULL){
        return MCE_INVALID_INPUT;
    }

    switch(window->type){
    case REFERENCE_FOREIGN: return foreign_is_system((MC_ForeignWindow*)window, out);
    case REFERENCE_INTERNAL: *out = false; return MCE_OK;
    default: return MCE_NOT_SUPPORTED;
    }
}

static MC_Error alloc_foreign(MC_WM *wm, MC_ForeignWindow **out){
    MC_WMVtab *v = &wm->vtab;

    MC_ForeignWindow *foreign;
    MC_RETURN_ERROR(mc_alloc(NULL, sizeof(MC_ForeignWindow) + v->foreign_window_size, (void**)&foreign));

    memset(foreign, 0, sizeof(MC_ForeignWindow) + v->foreign_window_size);
    foreign->ref.type = REFERENCE_FOREIGN;
    foreign->ref.refcount = 0;
    foreign->wm = wm;
    foreign->target = (struct MC_TargetForeignWindow*)foreign->data;

    *out = foreign;
    return MCE_OK;
}

static MC_Error track_foreign(MC_WM *wm, MC_ForeignWindow *foreign){
    ForeignWindows *new_foreign = MC_VECTOR_PUSHN(wm->foreign_windows, 1, &foreign);
    if(new_foreign == NULL){
        return MCE_OUT_OF_MEMORY;
    }
    wm->foreign_windows = new_foreign;

    return MCE_OK;
}

static bool is_ref_busy(MC_WindowRef *ref){
    return ref->type == REFERENCE_INTERNAL && !((MC_WMWindow*)ref)->is_alive;
}

static void window_free(MC_WMWindow *window){
    MC_WM *wm = window->wm;

    size_t idx = 0;
    MC_WMWindow **w;
    MC_VECTOR_EACH(wm->windows, w){
        if(*w == window){
            break;
        }

        idx++;
    }

    MC_VECTOR_ERASE(wm->windows, idx, 1);

    mc_free(NULL, window);
}

static void foreign_free(MC_ForeignWindow *foreign){
    MC_WM *wm = foreign->wm;

    if(wm->vtab.destroy_foreign_window){
        wm->vtab.destroy_foreign_window(wm->target, foreign->target);
    }

    size_t idx = 0;
    MC_ForeignWindow **w;
    MC_VECTOR_EACH(wm->foreign_windows, w){
        if(*w == foreign){
            break;
        }

        idx++;
    }

    MC_VECTOR_ERASE(wm->foreign_windows, idx, 1);

    mc_free(NULL, foreign);
}

static MC_Error owner_set_title(MC_WMWindow *window, MC_Str title){
    MC_WM *wm = window->wm;
    MC_WMVtab *v = &wm->vtab;

    if(v->set_window_title){
        return v->set_window_title(wm->target, window->target, title);
    }

    return MCE_NOT_SUPPORTED;
}

static MC_Error owner_get_title(MC_WMWindow *window, char *utf8, size_t cap, size_t *len){
    MC_WM *wm = window->wm;
    MC_WMVtab *v = &wm->vtab;

    if(v->get_window_title){
        return v->get_window_title(wm->target, window->target, utf8, cap, len);
    }

    return MCE_NOT_SUPPORTED;
}

static MC_Error owner_set_rect(MC_WMWindow *window, MC_WMArea area, MC_Rect2IU rect){
    MC_WM *wm = window->wm;
    MC_WMVtab *v = &wm->vtab;

    if(v->set_window_rect){
        MC_Error status = v->set_window_rect(wm->target, window->target, area, rect);
        if(status == MCE_OK){
            window->cached.rect[area] = rect;
        }

        return status;
    }

    return MCE_NOT_SUPPORTED;
}

static MC_Error owner_close(MC_WMWindow *window){
    mc_wm_window_destroy(window);

    return MCE_OK;
}

static MC_Error owner_focus(MC_WMWindow *window){
    MC_WM *wm = window->wm;
    MC_WMVtab *v = &wm->vtab;

    if(v->focus_window){
        return v->focus_window(wm->target, window->target);
    }

    return MCE_NOT_SUPPORTED;
}

static MC_Error owner_set_state(MC_WMWindow *window, MC_WMWindowState state){
    MC_WM *wm = window->wm;
    MC_WMVtab *v = &wm->vtab;

    if(v->set_window_state){
        return v->set_window_state(wm->target, window->target, state);
    }

    return MCE_NOT_SUPPORTED;
}

static MC_Error owner_get_rect(MC_WMWindow *window, MC_WMArea area, MC_Rect2IU *rect){
    MC_WM *wm = window->wm;
    MC_WMVtab *v = &wm->vtab;

    if(v->get_window_rect){
        MC_Error status = v->get_window_rect(wm->target, window->target, area, rect);
        if(status == MCE_OK){
            window->cached.rect[area] = *rect;
        }

        return status;
    }

    return MCE_NOT_SUPPORTED;
}

static MC_Error owner_get_state(MC_WMWindow *window, MC_WMWindowState *state){
    MC_WM *wm = window->wm;
    MC_WMVtab *v = &wm->vtab;

    if(v->get_window_state){
        MC_Error status = v->get_window_state(wm->target, window->target, state);
        if(status == MCE_OK){
            window->cached.state = *state;
        }

        return status;
    }

    return MCE_NOT_SUPPORTED;
}

static MC_Error foreign_set_title(MC_ForeignWindow *foreign, MC_Str title){
    MC_WM *wm = foreign->wm;

    if(wm->vtab.set_foreign_window_title){
        return wm->vtab.set_foreign_window_title(wm->target, foreign->target, title);
    }

    return MCE_NOT_SUPPORTED;
}

static MC_Error foreign_set_rect(MC_ForeignWindow *foreign, MC_WMArea area, MC_Rect2IU rect){
    MC_WM *wm = foreign->wm;

    if(wm->vtab.set_foreign_window_rect){
        return wm->vtab.set_foreign_window_rect(wm->target, foreign->target, area, rect);
    }

    return MCE_NOT_SUPPORTED;
}

static MC_Error foreign_close(MC_ForeignWindow *foreign){
    MC_WM *wm = foreign->wm;

    if(wm->vtab.close_foreign_window){
        return wm->vtab.close_foreign_window(wm->target, foreign->target);
    }

    return MCE_NOT_SUPPORTED;
}

static MC_Error foreign_focus(MC_ForeignWindow *foreign){
    MC_WM *wm = foreign->wm;

    if(wm->vtab.focus_foreign_window){
        return wm->vtab.focus_foreign_window(wm->target, foreign->target);
    }

    return MCE_NOT_SUPPORTED;
}

static MC_Error foreign_set_state(MC_ForeignWindow *foreign, MC_WMWindowState state){
    MC_WM *wm = foreign->wm;

    if(wm->vtab.set_foreign_window_state){
        return wm->vtab.set_foreign_window_state(wm->target, foreign->target, state);
    }

    return MCE_NOT_SUPPORTED;
}

static MC_Error foreign_get_title(MC_ForeignWindow *foreign, char *utf8, size_t cap, size_t *len){
    MC_WM *wm = foreign->wm;

    if(wm->vtab.get_foreign_window_title){
        return wm->vtab.get_foreign_window_title(wm->target, foreign->target, utf8, cap, len);
    }

    return MCE_NOT_SUPPORTED;
}

static MC_Error foreign_get_rect(MC_ForeignWindow *foreign, MC_WMArea area, MC_Rect2IU *rect){
    MC_WM *wm = foreign->wm;

    if(wm->vtab.get_foreign_window_rect){
        return wm->vtab.get_foreign_window_rect(wm->target, foreign->target, area, rect);
    }

    return MCE_NOT_SUPPORTED;
}

static MC_Error foreign_get_state(MC_ForeignWindow *foreign, MC_WMWindowState *state){
    MC_WM *wm = foreign->wm;

    if(wm->vtab.get_foreign_window_state){
        return wm->vtab.get_foreign_window_state(wm->target, foreign->target, state);
    }

    return MCE_NOT_SUPPORTED;
}

static MC_Error foreign_is_system(MC_ForeignWindow *foreign, bool *out){
    MC_WM *wm = foreign->wm;

    if(wm->vtab.is_foreign_window_system){
        return wm->vtab.is_foreign_window_system(wm->target, foreign->target, out);
    }

    return MCE_NOT_SUPPORTED;
}

static EventNode *queue_push(MC_WM *wm, const MC_WMEvent *event){
    EventNode *node = NULL;
    if(mc_alloc(wm->event_alloc, sizeof(EventNode), (void**)&node) != MCE_OK){
        return NULL;
    }

    node->next = NULL;
    node->event = *event;

    if(wm->event_queue_tail != NULL){
        wm->event_queue_tail->next = node;
    }
    else{
        wm->event_queue = node;
    }
    wm->event_queue_tail = node;

    if(wm->event_queue_mark == NULL){
        wm->event_queue_mark = node;
    }

    wm->event_count++;
    return node;
}

static void intern_event(MC_WM *wm, MC_WMEvent *event){
    if(event->type != MC_WME_PASTE_TEXT){
        return;
    }

    MC_Str text = event->as.paste_text.text;
    size_t len = (size_t)MC_STR_LEN(text);

    char *copy = NULL;
    if(len > 0 && mc_alloc(wm->event_alloc, len, (void**)&copy) == MCE_OK){
        memcpy(copy, text.beg, len);
        event->as.paste_text.text = MC_STR(copy, copy + len);
    }
    else{
        event->as.paste_text.text = MC_STR(NULL, NULL);
    }
}

static void queue_push_target(MC_WM *wm, MC_WMEvent *event){
    if(wm->event_count >= wm->event_limit * 2){
        WM_LOGE("event queue overflow (2x limit = %zu): dropping target event", wm->event_limit * 2);
        return;
    }

    intern_event(wm, event);
    queue_push(wm, event);
}

static void drain_target_events(MC_WM *wm){
    MC_WMVtab *v = &wm->vtab;

    for(;;){
        while(wm->pending_indications){
            if(indication_category[wm->indications[0].type] & wm->events){
                MC_WMEvent target_event = translate_indication(wm);
                queue_push_target(wm, &target_event);
            }
            else{
                discard_indication(wm);
            }
        }

        mc_arena_reset(wm->arena);

        if(v->heartbeat){
            v->heartbeat(wm->target);
        }

        if(!(v->poll_event && v->poll_event(wm->target, wm->target_event))){
            return;
        }

        if(v->translate_event){
            unsigned new_indications = v->translate_event(
                wm->target, wm->target_event,
                wm->indications + wm->pending_indications);

            MC_ASSERT_FAULT(new_indications <= MC_WM_MAX_INDICATIONS_PER_EVENT);

            wm->pending_indications += new_indications;
        }

        if(wm->pending_indications){
            handle_duplicate_indications(wm);
        }

        if(wm->events & MC_WM_EVENTS_RAW){
            void *raw = NULL;
            if(mc_alloc(wm->event_alloc, wm->vtab.event_size, &raw) == MCE_OK){
                memcpy(raw, wm->target_event, wm->vtab.event_size);
                MC_WMEvent raw_event = { .type = MC_WME_RAW, .as.raw = raw };
                queue_push_target(wm, &raw_event);
            }
        }
    }
}

static void drain_pending_user(MC_WM *wm){
    if(wm->pending_user == NULL){
        return;
    }

    if(wm->event_queue_tail != NULL){
        wm->event_queue_tail->next = wm->pending_user;
    }
    else{
        wm->event_queue = wm->pending_user;
    }
    wm->event_queue_tail = wm->pending_user_tail;

    if(wm->event_queue_mark == NULL){
        wm->event_queue_mark = wm->pending_user;
    }

    wm->pending_user = NULL;
    wm->pending_user_tail = NULL;
}

MC_Error mc_wm_poll_event(MC_WM *wm, MC_WMEvent *event){
    if(!wm->is_alive){
        return MCE_BUSY;
    }

    drain_target_events(wm);
    drain_pending_user(wm);

    if(wm->event_queue == NULL){
        return MCE_AGAIN;
    }

    EventNode *node = wm->event_queue;
    wm->event_queue = node->next;
    if(wm->event_queue == NULL){
        wm->event_queue_tail = NULL;
    }

    *event = node->event;
    wm->event_count--;

    if(node == wm->event_queue_mark){
        mc_dbarena_swap(wm->event_arena);
        wm->event_queue_mark = NULL;
    }

    return MCE_OK;
}

static bool event_matches(const MC_WMEventMatch *match, const MC_WMEvent *event){
    return match->type == MC_WME_NONE || match->type == event->type;
}

MC_Error mc_wm_subscribe_event(MC_WMRef *ref, MC_WMEventMatch match, void (*callback)(MC_WMRef *wm, const MC_WMEvent *event, void *user_data), void *user_data, MC_WMEventSubscription **out){
    if(ref == NULL || callback == NULL){
        return MCE_INVALID_INPUT;
    }

    RETURN_IF_WM_BUSY(ref);

    MC_WM *wm = wm_of(ref);

    MC_WMEventSubscription *sub;
    MC_RETURN_ERROR(mc_alloc(NULL, sizeof(*sub), (void**)&sub));

    sub->next = wm->event_subs;
    sub->wm = wm;
    sub->match = match;
    sub->callback = callback;
    sub->user_data = user_data;
    wm->event_subs = sub;

    if(out != NULL){
        *out = sub;
    }

    return MCE_OK;
}

void mc_wm_unsubscribe_event(MC_WMEventSubscription *subscription){
    if(subscription == NULL){
        return;
    }

    MC_WMEventSubscription **link = &subscription->wm->event_subs;
    while(*link != NULL){
        if(*link == subscription){
            *link = subscription->next;
            break;
        }

        link = &(*link)->next;
    }

    mc_free(NULL, subscription);
}

void mc_wm_dispatch_event_callbacks(MC_WMRef *ref, const MC_WMEvent *event){
    if(ref == NULL || event == NULL){
        return;
    }

    MC_WM *wm = wm_of(ref);
    if(!wm->is_alive){
        return;
    }

    MC_WMEventSubscription *sub = wm->event_subs;
    while(sub != NULL){
        MC_WMEventSubscription *next = sub->next;
        if(event_matches(&sub->match, event)){
            sub->callback(ref, event, sub->user_data);
        }

        sub = next;
    }
}

MC_Alloc *mc_wm_event_allocator(MC_WMRef *ref){
    if(ref == NULL){
        return NULL;
    }

    return wm_of(ref)->event_alloc;
}

static EventGroup *find_group(MC_WM *wm, MC_WMEventType type){
    EventGroup *g;
    MC_VECTOR_EACH(wm->event_groups, g){
        if(type >= g->offset && (size_t)(type - g->offset) < g->size){
            return g;
        }
    }

    return NULL;
}

static EventGroup *find_group_by_name(MC_WM *wm, const char *name){
    EventGroup *g;
    MC_VECTOR_EACH(wm->event_groups, g){
        if(strcmp(g->name->data, name) == 0){
            return g;
        }
    }

    return NULL;
}

MC_Error mc_wm_register_event_group(MC_WMRef *ref, const MC_WMEventGroupDef *def, MC_WMEventType *out_offset){
    if(ref == NULL || def == NULL || def->name == NULL || def->events == NULL || def->size == 0 || out_offset == NULL){
        return MCE_INVALID_INPUT;
    }

    MC_WM *wm = wm_of(ref);

    size_t total = def->size + def->reserve;
    if((size_t)wm->event_next + total > 0xFFFF){
        return MCE_OVERFLOW;
    }

    if(find_group_by_name(wm, def->name) != NULL){
        return MCE_INVALID_INPUT;
    }

    MC_WMEventType offset = wm->event_next;

    MC_String *name = NULL;
    MC_RETURN_ERROR(mc_string_fmt(NULL, &name, "%s", def->name));

    MC_WMEventDefinition *events = NULL;
    MC_Error status = mc_alloc(NULL, def->size * sizeof(MC_WMEventDefinition), (void**)&events);
    if(status != MCE_OK){
        mc_free(NULL, name);
        return status;
    }

    size_t group_len = strlen(def->name);
    for(size_t i = 0; i < def->size; i++){
        size_t event_len = strlen(def->events[i].name);
        char *serial = NULL;
        status = mc_alloc(NULL, group_len + 1 + event_len + 1, (void**)&serial);
        if(status != MCE_OK){
            for(size_t j = 0; j < i; j++){
                mc_free(NULL, (void*)events[j].name);
            }
            mc_free(NULL, events);
            mc_free(NULL, name);
            return status;
        }

        memcpy(serial, def->name, group_len);
        serial[group_len] = '.';
        memcpy(serial + group_len + 1, def->events[i].name, event_len);
        serial[group_len + 1 + event_len] = '\0';

        events[i] = def->events[i];
        events[i].name = serial;
    }

    EventGroup entry = {
        .offset = offset, .size = def->size, .reserve = def->reserve,
        .name = name, .events = events,
        .to_json = def->to_json, .from_json = def->from_json,
    };
    EventGroups *grown = MC_VECTOR_PUSHN(wm->event_groups, 1, (&entry));
    if(grown == NULL){
        for(size_t i = 0; i < def->size; i++){
            mc_free(NULL, (void*)events[i].name);
        }
        mc_free(NULL, events);
        mc_free(NULL, name);
        return MCE_OUT_OF_MEMORY;
    }
    wm->event_groups = grown;
    wm->event_next = (MC_WMEventType)(offset + total);

    *out_offset = offset;
    return MCE_OK;
}

MC_WMEventType mc_wm_event_group_offset(MC_WMRef *ref, const char *group){
    if(ref == NULL || group == NULL){
        return MC_WME_NONE;
    }

    EventGroup *g = find_group_by_name(wm_of(ref), group);
    return g != NULL ? g->offset : MC_WME_NONE;
}

MC_Error mc_wm_event(MC_WMRef *ref, MC_WMEventType type, MC_WMEvent *out){
    if(ref == NULL || out == NULL){
        return MCE_INVALID_INPUT;
    }

    MC_WM *wm = wm_of(ref);

    if(find_group(wm, type) == NULL){
        return MCE_NOT_FOUND;
    }

    MC_Json *data;
    MC_RETURN_ERROR(mc_json_new(wm->event_alloc, &data));

    *out = (MC_WMEvent){ .type = type };
    out->as.raw = data;
    return MCE_OK;
}

MC_Error mc_wm_push_event(MC_WMRef *ref, const MC_WMEvent *event){
    if(ref == NULL || event == NULL){
        return MCE_INVALID_INPUT;
    }

    MC_WM *wm = wm_of(ref);

    if(wm->event_count >= wm->event_limit){
        return MCE_OVERFLOW;
    }

    EventNode *node = NULL;
    MC_RETURN_ERROR(mc_alloc(wm->event_alloc, sizeof(EventNode), (void**)&node));

    node->next = NULL;
    node->event = *event;

    if(wm->pending_user_tail != NULL){
        wm->pending_user_tail->next = node;
    }
    else{
        wm->pending_user = node;
    }
    wm->pending_user_tail = node;

    wm->event_count++;
    return MCE_OK;
}

void mc_wm_set_event_queue_limit(MC_WMRef *ref, size_t limit){
    if(ref == NULL){
        return;
    }

    wm_of(ref)->event_limit = limit;
}

const char *mc_wm_event_type_str(MC_WMRef *ref, MC_WMEventType type){
    if(ref == NULL){
        return NULL;
    }

    EventGroup *g = find_group(wm_of(ref), type);
    return g != NULL ? g->events[type - g->offset].name : NULL;
}

MC_WMEventType mc_wm_event_type_from_str(MC_WMRef *ref, const char *name){
    if(ref == NULL || name == NULL){
        return MC_WME_NONE;
    }

    MC_WM *wm = wm_of(ref);
    EventGroup *g;
    MC_VECTOR_EACH(wm->event_groups, g){
        for(size_t i = 0; i < g->size; i++){
            if(strcmp(g->events[i].name, name) == 0){
                return (MC_WMEventType)(g->offset + i);
            }
        }
    }

    return MC_WME_NONE;
}

static MC_Json *json_field(const MC_Json *json, const char *key){
    size_t n = mc_json_length((MC_Json*)json);
    size_t key_len = strlen(key);
    for(size_t i = 0; i < n; i++){
        MC_Str k;
        MC_Json *v;
        if(mc_json_object_at((MC_Json*)json, i, &k, &v) != MCE_OK){
            continue;
        }

        if((size_t)MC_STR_LEN(k) == key_len && memcmp(k.beg, key, key_len) == 0){
            return v;
        }
    }

    return NULL;
}

MC_Error mc_wm_event_to_json(MC_WMRef *ref, const MC_WMEvent *event, MC_Json *out){
    if(event == NULL || out == NULL){
        return MCE_INVALID_INPUT;
    }

    MC_RETURN_ERROR(mc_json_set_object(out));

    const char *type_name = mc_wm_event_type_str(ref, event->type);
    MC_RETURN_ERROR(mc_json_object_add_str(out, mc_strc(type_name != NULL ? type_name : "UNKNOWN"), "type"));

    EventGroup *g = ref != NULL ? find_group(wm_of(ref), event->type) : NULL;

    if(g != NULL && g->to_json != NULL){
        return g->to_json(NULL, event, out);
    }

    if(event->as.raw != NULL){
        MC_Json *data;
        MC_RETURN_ERROR(mc_json_object_add_new(out, &data, "data"));
        return mc_json_copy(data, (MC_Json*)event->as.raw);
    }

    return MCE_OK;
}

MC_Error mc_wm_event_from_json(MC_WMRef *ref, MC_Alloc *alloc, const MC_Json *json, MC_WMEvent *out){
    if(ref == NULL || json == NULL || out == NULL){
        return MCE_INVALID_INPUT;
    }

    MC_Json *type_node = json_field(json, "type");
    MC_Str type_str;
    if(type_node == NULL || mc_json_str(type_node, &type_str) != MCE_OK){
        return MCE_INVALID_INPUT;
    }

    char buf[128];
    size_t len = (size_t)MC_STR_LEN(type_str);
    if(len >= sizeof(buf)){
        return MCE_INVALID_INPUT;
    }
    memcpy(buf, type_str.beg, len);
    buf[len] = '\0';

    MC_WMEventType type = mc_wm_event_type_from_str(ref, buf);
    if(type == MC_WME_NONE){
        return MCE_NOT_FOUND;
    }

    *out = (MC_WMEvent){ .type = type };

    EventGroup *g = find_group(wm_of(ref), type);
    if(g != NULL && g->from_json != NULL){
        return g->from_json(alloc, json, out);
    }

    MC_Json *data;
    MC_RETURN_ERROR(mc_json_new(alloc, &data));
    MC_Json *data_node = json_field(json, "data");
    if(data_node != NULL){
        MC_Error e = mc_json_copy(data, data_node);
        if(e != MCE_OK){
            mc_json_delete(&data);
            return e;
        }
    }
    out->as.raw = data;

    return MCE_OK;
}

static void dump_vtable(MC_WM *wm){
    WM_LOG_DEV("wm_size: %zu", wm->vtab.wm_size);
    WM_LOG_DEV("window_size: %zu", wm->vtab.window_size);
    WM_LOG_DEV("event_size: %zu", wm->vtab.event_size);

    #define DUMP_IF_NOT_IMPLEMENTED(METHOD) if(!wm->vtab.METHOD) WM_LOG_DEV("%s() is not implemented", #METHOD)

    DUMP_IF_NOT_IMPLEMENTED(init);
    DUMP_IF_NOT_IMPLEMENTED(destroy);

    DUMP_IF_NOT_IMPLEMENTED(init_window);
    DUMP_IF_NOT_IMPLEMENTED(destroy_window);

    DUMP_IF_NOT_IMPLEMENTED(set_window_title);
    DUMP_IF_NOT_IMPLEMENTED(set_window_rect);
    DUMP_IF_NOT_IMPLEMENTED(set_window_state);
    DUMP_IF_NOT_IMPLEMENTED(get_window_title);
    DUMP_IF_NOT_IMPLEMENTED(get_window_rect);


    DUMP_IF_NOT_IMPLEMENTED(poll_event);
    DUMP_IF_NOT_IMPLEMENTED(translate_event);

    #undef DUMP_IF_NOT_IMPLEMENTED
}

static void handle_duplicate_indications(MC_WM *wm){
    static const MC_TargetIndicationDuplicateAction dup_action[MC_WMIND_COUNT] = {
        #define MC_WMIDN(NAME, DUP_ACTION, ...) [MC_WMIND_##NAME] = MC_WM_INDDUP_##DUP_ACTION,
        MC_ITER_INDICATIONS()
        #undef MC_WMIDN
    };

    bool to_remove[INDICATIONS_BUFFER_SIZE] = {0};
    int prev[MC_WMIND_COUNT];

    for(int i = 0; i < MC_WMIND_COUNT; i++){
        prev[i] = -1;
    }

    for(unsigned idx = 0; idx < wm->pending_indications; idx++){
        MC_ASSERT_FAULT(wm->indications[idx].type < MC_WMIND_COUNT);

        if(prev[wm->indications[idx].type] == -1){
            prev[wm->indications[idx].type] = idx;
            continue;
        }

        switch(dup_action[wm->indications[idx].type]){
        case MC_WM_INDDUP_USE_ALL:
            break;
        case MC_WM_INDDUP_USE_LAST:
            to_remove[prev[wm->indications[idx].type]] = true;
            break;
        case MC_WM_INDDUP_UNIQUE_PER_WINDOW:
            MC_ASSERT_FAULT("UNIQUE_PER_WINDOW was received twice" && 0);
            break;
        }

        prev[wm->indications[idx].type] = idx;
    }

    for(int idx = wm->pending_indications - 1; idx >= 0; idx--){
        if(to_remove[idx]){
            // TODO: consider using a list
            memmove(&wm->indications[idx], &wm->indications[idx + 1],
                sizeof(MC_TargetIndication[--wm->pending_indications - idx]));
        }
    }

    MC_ASSERT_FAULT(wm->pending_indications);
}

static void discard_indication(MC_WM *wm){
    memmove(wm->indications, wm->indications + 1, sizeof(MC_TargetIndication[--wm->pending_indications]));
}

static uint64_t window_identity(MC_WMWindow *window){
    return window ? window->parameters.identity : 0;
}

static MC_WMEvent translate_indication(MC_WM *wm){
    MC_TargetIndication ind = *wm->indications;
    memmove(wm->indications, wm->indications + 1, sizeof(MC_TargetIndication[--wm->pending_indications]));

    MC_WMWindow *window;

    switch (ind.type){
    case MC_WMIND_WINDOW_READY:
        window = window_from_target(wm, ind.as.window_ready.window);

        return (MC_WMEvent){
            .type = MC_WME_WINDOW_READY,
            .as.window_ready = {
                .window = window_identity(window),
            }
        };
    case MC_WMIND_WINDOW_MOVED:
        window = window_from_target(wm, ind.as.window_moved.window);

        return (MC_WMEvent){
            .type = MC_WME_WINDOW_MOVED,
            .as.window_moved = {
                .window = window_identity(window),
                .new_position = ind.as.window_moved.new_position,
            }
        };
    case MC_WMIND_WINDOW_RESIZED:
        window = window_from_target(wm, ind.as.window_resized.window);

        return (MC_WMEvent){
            .type = MC_WME_WINDOW_RESIZED,
            .as.window_resized = {
                .window = window_identity(window),
                .new_size = ind.as.window_resized.new_size,
            }
        };
    case MC_WMIND_WINDOW_REDRAW_REQUESTED:
        window = window_from_target(wm, ind.as.redraw_requested.window);

        return (MC_WMEvent){
            .type = MC_WME_WINDOW_REDRAW_REQUESTED,
            .as.redraw_requested = {
                .window = window_identity(window),
            }
        };
    case MC_WMIND_WINDOW_CLOSE_REQUESTED:
        window = window_from_target(wm, ind.as.window_close_requested.window);

        return (MC_WMEvent){
            .type = MC_WME_WINDOW_CLOSE_REQUESTED,
            .as.window_close_requested = {
                .window = window_identity(window),
            }
        };
    case MC_WMIND_WINDOW_STATE_CHANGED:
        window = window_from_target(wm, ind.as.window_state_changed.window);
        window->cached.state = ind.as.window_state_changed.state;

        return (MC_WMEvent){
            .type = MC_WME_WINDOW_STATE_CHANGED,
            .as.window_state_changed = {
                .window = window_identity(window),
                .state = ind.as.window_state_changed.state,
            }
        };
    case MC_WMIND_FOCUS_GAINED:
        window = window_from_target(wm, ind.as.focus_gained.window);

        return (MC_WMEvent){
            .type = MC_WME_FOCUS_GAINED,
            .as.focus_gained = {
                .window = window_identity(window),
            }
        };
    case MC_WMIND_FOCUS_LOST:
        window = window_from_target(wm, ind.as.focus_lost.window);

        return (MC_WMEvent){
            .type = MC_WME_FOCUS_LOST,
            .as.focus_lost = {
                .window = window_identity(window),
            }
        };
    case MC_WMIND_MOUSE_DOWN:
        window = window_from_target(wm, ind.as.mouse_down.window);

        return (MC_WMEvent){
            .type = MC_WME_MOUSE_DOWN,
            .as.mouse_down = {
                .window = window_identity(window),
                .button = ind.as.mouse_down.button,
                .position = window->cached.mouse_pos,
            }
        };
    case MC_WMIND_MOUSE_UP:
        window = window_from_target(wm, ind.as.mouse_up.window);

        return (MC_WMEvent){
            .type = MC_WME_MOUSE_UP,
            .as.mouse_up = {
                .window = window_identity(window),
                .button = ind.as.mouse_up.button,
                .position = window->cached.mouse_pos,
            }
        };
    case MC_WMIND_MOUSE_MOVED:
        window = window_from_target(wm, ind.as.mouse_moved.window);
        window->cached.mouse_pos = ind.as.mouse_moved.position;

        return (MC_WMEvent){
            .type = MC_WME_MOUSE_MOVED,
            .as.mouse_moved = {
                .window = window_identity(window),
                .position = ind.as.mouse_moved.position,
            }
        };
    case MC_WMIND_MOUSE_ENTER:
        window = window_from_target(wm, ind.as.mouse_enter.window);
        window->cached.mouse_over = true;

        return (MC_WMEvent){
            .type = MC_WME_MOUSE_ENTER,
            .as.mouse_enter = {
                .window = window_identity(window),
                .position = window->cached.mouse_pos,
            }
        };
    case MC_WMIND_MOUSE_LEAVE:
        window = window_from_target(wm, ind.as.mouse_leave.window);
        window->cached.mouse_over = false;

        return (MC_WMEvent){
            .type = MC_WME_MOUSE_LEAVE,
            .as.mouse_enter = {
                .window = window_identity(window),
                .position = window->cached.mouse_pos,
            }
        };
    case MC_WMIND_KEY_DOWN:
        return (MC_WMEvent){
            .type = MC_WME_KEY_DOWN,
            .as.key_down = {
                .window = 0,
                .key = ind.as.key_down.key,
            }
        };
    case MC_WMIND_KEY_UP:
        return (MC_WMEvent){
            .type = MC_WME_KEY_UP,
            .as.key_up = {
                .window = 0,
                .key = ind.as.key_up.key,
            }
        };
    case MC_WMIND_TEXT_INPUT:
        window = window_from_target(wm, ind.as.text_input.window);

        {
            MC_WMEvent event = {
                .type = MC_WME_TEXT_INPUT,
                .as.text_input = {
                    .window = window_identity(window),
                }
            };

            memcpy(event.as.text_input.utf8, ind.as.text_input.utf8, MC_WM_TEXT_INPUT_CAP);
            event.as.text_input.utf8[MC_WM_TEXT_INPUT_CAP - 1] = '\0';
            return event;
        }
    case MC_WMIND_PASTE_TEXT:
        window = window_from_target(wm, ind.as.paste_text.window);

        return (MC_WMEvent){
            .type = MC_WME_PASTE_TEXT,
            .as.paste_text = {
                .window = window_identity(window),
                .text = ind.as.paste_text.text,
            }
        };
    case MC_WMIND_GLOBAL_KEY_DOWN:
        return (MC_WMEvent){
            .type = MC_WME_GLOBAL_KEY_DOWN,
            .as.global_key_down = {.key = ind.as.global_key_down.key},
        };
    case MC_WMIND_GLOBAL_KEY_UP:
        return (MC_WMEvent){
            .type = MC_WME_GLOBAL_KEY_UP,
            .as.global_key_up = {.key = ind.as.global_key_up.key},
        };
    case MC_WMIND_GLOBAL_MOUSE_MOVED:
        return (MC_WMEvent){
            .type = MC_WME_GLOBAL_MOUSE_MOVED,
            .as.global_mouse_moved = {.position = ind.as.global_mouse_moved.position},
        };
    case MC_WMIND_GLOBAL_MOUSE_DOWN:
        return (MC_WMEvent){
            .type = MC_WME_GLOBAL_MOUSE_DOWN,
            .as.global_mouse_down = {
                .position = ind.as.global_mouse_down.position,
                .button = ind.as.global_mouse_down.button,
            },
        };
    case MC_WMIND_GLOBAL_MOUSE_UP:
        return (MC_WMEvent){
            .type = MC_WME_GLOBAL_MOUSE_UP,
            .as.global_mouse_up = {
                .position = ind.as.global_mouse_up.position,
                .button = ind.as.global_mouse_up.button,
            },
        };
    case MC_WMIND_GLOBAL_MOUSE_WHEEL:
        return (MC_WMEvent){
            .type = MC_WME_GLOBAL_MOUSE_WHEEL,
            .as.global_mouse_wheel = {
                .position = ind.as.global_mouse_wheel.position,
                .up = ind.as.global_mouse_wheel.up,
                .right = ind.as.global_mouse_wheel.right,
            },
        };
    default:
        MC_ASSERT_FAULT("NOT IMPLEMENTED YET" && 0);
    }
}

static MC_WMWindow *window_from_target(MC_WM *wm, struct MC_TargetWMWindow *target){
    if(target == NULL){
        if(MC_VECTOR_SIZE(wm->windows) == 1){
            return *wm->windows->beg;
        }

        return NULL;
    }

    MC_WMWindow **w;
    MC_VECTOR_EACH(wm->windows, w){
        if((*w)->target == target){
            return *w;
        }
    }

    return NULL;
}

static MC_WMWindow *find_window(MC_WM *wm, uint64_t identity){
    MC_WMWindow **w;
    MC_VECTOR_EACH(wm->windows, w){
        if((*w)->parameters.identity == identity){
            return *w;
        }
    }

    return NULL;
}

static MC_ForeignWindow *find_foreign(MC_WM *wm, uint64_t identity){
    MC_ForeignWindow **w;
    MC_VECTOR_EACH(wm->foreign_windows, w){
        if((*w)->identity == identity){
            return *w;
        }
    }

    return NULL;
}
