#include <mc/wm/wm.h>
#include <mc/wm/target.h>
#include <mc/wm/event.h>

#include <mc/data/string.h>
#include <mc/data/vector.h>
#include <mc/data/arena.h>

#include <mc/os/file.h>
#include <mc/util/assert.h>
#include <mc/util/memory.h>
#include <mc/util/error.h>

#include <malloc.h>
#include <memory.h>

#define INDICATIONS_BUFFER_SIZE (MC_WM_MAX_INDICATIONS_PER_EVENT * 16)

#define WM_LOGE(FMT, ...) if(wm->log) mc_fmt(wm->log, "[WM] " FMT "\n", ##__VA_ARGS__)
#define WM_LOG_DEV(FMT, ...) if(wm->log) mc_fmt(wm->log, "%s() [DEV][WM][%s] " FMT "\n", __func__, wm->vtab.name, ##__VA_ARGS__)

#define RETURN_IF_REF_BUSY(REF) \
    do { \
        if(is_ref_busy(REF)){ \
            return MCE_BUSY; \
        } \
    } while(0)

MC_DEFINE_VECTOR(Windows, MC_WMWindow*);
MC_DEFINE_VECTOR(ForeignWindows, MC_ForeignWindow*);

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
    struct{
        MC_Rect2IU rect;
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

struct MC_WM{
    MC_WMVtab vtab;
    MC_Stream *log;
    struct MC_TargetWM *target;
    struct MC_TargetWMEvent *target_event;

    MC_WMEvents events;

    unsigned pending_indications;
    MC_TargetIndication indications[INDICATIONS_BUFFER_SIZE];

    Windows *windows;
    ForeignWindows *foreign_windows;

    MC_Arena *arena;

    alignas(void*) uint8_t data[];
};

static void dump_vtable(MC_WM *wm);
static void handle_duplicate_indications(MC_WM *wm);
static MC_WMEvent translate_indication(MC_WM *wm);
static void discard_indication(MC_WM *wm);
static MC_WMWindow *window_from_target(MC_WM *wm, struct MC_TargetWMWindow *target);
static MC_ForeignWindow *find_foreign(MC_WM *wm, uint64_t identity);
static bool is_ref_busy(MC_WindowRef *ref);
static MC_Error alloc_foreign(MC_WM *wm, MC_ForeignWindow **out);
static MC_Error track_foreign(MC_WM *wm, MC_ForeignWindow *foreign);
static void window_free(MC_WMWindow *window);
static void foreign_free(MC_ForeignWindow *foreign);

static MC_Error owner_set_title(MC_WMWindow *window, MC_Str title);
static MC_Error owner_set_position(MC_WMWindow *window, MC_Vec2i position);
static MC_Error owner_set_size(MC_WMWindow *window, MC_Size2U size);
static MC_Error owner_set_rect(MC_WMWindow *window, MC_Rect2IU rect);
static MC_Error owner_set_state(MC_WMWindow *window, MC_WMWindowState state);
static MC_Error owner_get_position(MC_WMWindow *window, MC_Vec2i *position);
static MC_Error owner_get_size(MC_WMWindow *window, MC_Size2U *size);
static MC_Error owner_get_rect(MC_WMWindow *window, MC_Rect2IU *rect);

static MC_Error foreign_set_title(MC_ForeignWindow *foreign, MC_Str title);
static MC_Error foreign_set_position(MC_ForeignWindow *foreign, MC_Vec2i position);
static MC_Error foreign_set_size(MC_ForeignWindow *foreign, MC_Size2U size);
static MC_Error foreign_set_rect(MC_ForeignWindow *foreign, MC_Rect2IU rect);
static MC_Error foreign_set_state(MC_ForeignWindow *foreign, MC_WMWindowState state);
static MC_Error foreign_get_title(MC_ForeignWindow *foreign, char *utf8, size_t cap, size_t *len);
static MC_Error foreign_get_position(MC_ForeignWindow *foreign, MC_Vec2i *position);
static MC_Error foreign_get_size(MC_ForeignWindow *foreign, MC_Size2U *size);
static MC_Error foreign_get_rect(MC_ForeignWindow *foreign, MC_Rect2IU *rect);

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

    MC_Error arena_status = mc_arena_init(&wm->arena, NULL);
    if(arena_status != MCE_OK){
        mc_free(NULL, wm->target_event);
        mc_free(NULL, wm);
        return arena_status;
    }

    MC_Error init_status = vtab->init(wm->target, wm->log, mc_arena_allocator(wm->arena));
    if(init_status != MCE_OK){
        mc_arena_destroy(wm->arena);
        mc_free(NULL, wm->target_event);
        mc_free(NULL, wm);
        return init_status;
    }

    *ret_wm = wm;
    (void)dump_vtable;
    // dump_vtable(wm);

    return MCE_OK;
}

void mc_wm_destroy(MC_WM *wm){
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

    if(wm->vtab.destroy){
        wm->vtab.destroy(wm->target);
    }

    MC_VECTOR_FREE(wm->windows);
    MC_VECTOR_FREE(wm->foreign_windows);

    mc_arena_destroy(wm->arena);

    mc_free(NULL, wm);
}

struct MC_TargetWM *mc_wm_get_target(MC_WM *wm){
    return wm->target;
}

MC_Error mc_wm_request_events(MC_WM *wm, MC_WMEvents events){
    wm->events |= events;

    if(wm->vtab.request_events){
        return wm->vtab.request_events(wm->target, events);
    }

    return MCE_OK;
}

MC_WMEvents mc_wm_get_requested_events(MC_WM *wm){
    return wm->events;
}

MC_Error mc_wm_window_init(MC_WM *wm, MC_WMWindow **ret_window){
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

    MC_Error status = v->init_window(wm->target, window->target);
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

MC_Vec2i mc_wm_window_cached_get_position(MC_WMWindow *window){
    return (MC_Vec2i){
        .x = window->cached.rect.x,
        .y = window->cached.rect.y
    };
}

MC_Size2U mc_wm_window_cached_get_size(MC_WMWindow *window){
    return (MC_Size2U){
        .width = window->cached.rect.width,
        .height = window->cached.rect.height
    };
}

MC_Rect2IU mc_wm_window_cached_get_rect(MC_WMWindow *window){
    return window->cached.rect;
}

MC_WMWindowState mc_wm_window_cached_get_state(MC_WMWindow *window){
    return window->cached.state;
}

bool mc_wm_window_cached_is_mouse_over(MC_WMWindow *window){
    return window->cached.mouse_over;
}

MC_Error mc_wm_get_focused_window(MC_WM *wm, MC_WindowRef **ret_window){
    *ret_window = NULL;
    MC_WMVtab *v = &wm->vtab;

    if(v->get_focused_window == NULL || v->resolve_temporary_identity == NULL){
        return MCE_NOT_SUPPORTED;
    }

    uint64_t identity;
    MC_RETURN_ERROR(v->get_focused_window(wm->target, &identity));

    MC_ForeignWindow *existing = find_foreign(wm, identity);
    if(existing){
        existing->ref.refcount++;
        *ret_window = &existing->ref;
        return MCE_OK;
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

struct MC_TargetForeignWindow *mc_wm_window_get_foreign_target(MC_WindowRef *window){
    if(window->type == REFERENCE_FOREIGN){
        return ((MC_ForeignWindow*)window)->target;
    }

    return NULL;
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

MC_Error mc_wm_window_set_position(MC_WindowRef *window, MC_Vec2i position){
    RETURN_IF_REF_BUSY(window);

    switch(window->type){
    case REFERENCE_FOREIGN: return foreign_set_position((MC_ForeignWindow*)window, position);
    case REFERENCE_INTERNAL: return owner_set_position((MC_WMWindow*)window, position);
    default: return MCE_NOT_SUPPORTED;
    }
}

MC_Error mc_wm_window_set_size(MC_WindowRef *window, MC_Size2U size){
    RETURN_IF_REF_BUSY(window);

    switch(window->type){
    case REFERENCE_FOREIGN: return foreign_set_size((MC_ForeignWindow*)window, size);
    case REFERENCE_INTERNAL: return owner_set_size((MC_WMWindow*)window, size);
    default: return MCE_NOT_SUPPORTED;
    }
}

MC_Error mc_wm_window_set_rect(MC_WindowRef *window, MC_Rect2IU rect){
    RETURN_IF_REF_BUSY(window);

    switch(window->type){
    case REFERENCE_FOREIGN: return foreign_set_rect((MC_ForeignWindow*)window, rect);
    case REFERENCE_INTERNAL: return owner_set_rect((MC_WMWindow*)window, rect);
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
    default: return MCE_NOT_SUPPORTED;
    }
}

MC_Error mc_wm_window_get_position(MC_WindowRef *window, MC_Vec2i *position){
    RETURN_IF_REF_BUSY(window);

    switch(window->type){
    case REFERENCE_FOREIGN: return foreign_get_position((MC_ForeignWindow*)window, position);
    case REFERENCE_INTERNAL: return owner_get_position((MC_WMWindow*)window, position);
    default: return MCE_NOT_SUPPORTED;
    }
}

MC_Error mc_wm_window_get_size(MC_WindowRef *window, MC_Size2U *size){
    RETURN_IF_REF_BUSY(window);

    switch(window->type){
    case REFERENCE_FOREIGN: return foreign_get_size((MC_ForeignWindow*)window, size);
    case REFERENCE_INTERNAL: return owner_get_size((MC_WMWindow*)window, size);
    default: return MCE_NOT_SUPPORTED;
    }
}

MC_Error mc_wm_window_get_rect(MC_WindowRef *window, MC_Rect2IU *rect){
    RETURN_IF_REF_BUSY(window);

    switch(window->type){
    case REFERENCE_FOREIGN: return foreign_get_rect((MC_ForeignWindow*)window, rect);
    case REFERENCE_INTERNAL: return owner_get_rect((MC_WMWindow*)window, rect);
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

static MC_Error owner_set_position(MC_WMWindow *window, MC_Vec2i position){
    MC_WM *wm = window->wm;
    MC_WMVtab *v = &wm->vtab;

    MC_Error status = MCE_NOT_SUPPORTED;

    if(v->set_window_position){
        status = v->set_window_position(wm->target, window->target, position);
        if(status == MCE_OK){
            window->cached.rect.x = position.x;
            window->cached.rect.y = position.y;
            return MCE_OK;
        }
    }

    if(v->set_window_rect){
        status = v->set_window_rect(wm->target, window->target, (MC_Rect2IU){
            .x = position.x,
            .y = position.y,
            .width = window->cached.rect.width,
            .height = window->cached.rect.height
        });

        if(status == MCE_OK){
            window->cached.rect.x = position.x;
            window->cached.rect.y = position.y;
            return MCE_OK;
        }
    }

    return status;
}

static MC_Error owner_set_size(MC_WMWindow *window, MC_Size2U size){
    MC_WM *wm = window->wm;
    MC_WMVtab *v = &wm->vtab;

    MC_Error status = MCE_NOT_SUPPORTED;

    if(v->set_window_size){
        status = v->set_window_size(wm->target, window->target, size);
        if(status == MCE_OK){
            window->cached.rect.width = size.width;
            window->cached.rect.height = size.height;
            return MCE_OK;
        }
    }

    if(v->set_window_rect){
        status = v->set_window_rect(wm->target, window->target, (MC_Rect2IU){
            .x = window->cached.rect.x,
            .y = window->cached.rect.y,
            .width = size.width,
            .height = size.height
        });

        if(status == MCE_OK){
            window->cached.rect.width = size.width;
            window->cached.rect.height = size.height;
            return MCE_OK;
        }
    }

    return status;
}

static MC_Error owner_set_rect(MC_WMWindow *window, MC_Rect2IU rect){
    MC_WM *wm = window->wm;
    MC_WMVtab *v = &wm->vtab;

    MC_Error status = MCE_NOT_SUPPORTED;

    if(v->set_window_rect){
        status = v->set_window_rect(wm->target, window->target, rect);
        if(status == MCE_OK){
            window->cached.rect = rect;
            return MCE_OK;
        }
    }

    if(v->set_window_position && v->set_window_size){
        MC_Error position_status = v->set_window_position(wm->target, window->target, (MC_Vec2i){
            .x = rect.x,
            .y = rect.y,
        });

        MC_Error size_status = v->set_window_size(wm->target, window->target, (MC_Size2U){
            .width = rect.width,
            .height = rect.height,
        });

        if(position_status == MCE_OK){
            window->cached.rect.x = rect.x;
            window->cached.rect.y = rect.y;
        }

        if(size_status == MCE_OK){
            window->cached.rect.width = rect.width;
            window->cached.rect.height = rect.height;
        }

        if(size_status != MCE_OK){
            status = size_status;
        }
        else if(position_status != MCE_OK){
            status = position_status;
        }
    }

    return status;
}

static MC_Error owner_set_state(MC_WMWindow *window, MC_WMWindowState state){
    MC_WM *wm = window->wm;
    MC_WMVtab *v = &wm->vtab;

    if(v->set_window_state){
        return v->set_window_state(wm->target, window->target, state);
    }

    return MCE_NOT_SUPPORTED;
}

static MC_Error owner_get_position(MC_WMWindow *window, MC_Vec2i *position){
    MC_WM *wm = window->wm;
    MC_WMVtab *v = &wm->vtab;

    MC_Error status = MCE_NOT_SUPPORTED;

    if(v->get_window_position){
        MC_Error position_status = v->get_window_position(wm->target, window->target, position);
        if(position_status == MCE_OK){
            window->cached.rect.x = position->x;
            window->cached.rect.y = position->y;
            return MCE_OK;
        }

        status = position_status;
    }

    if(v->get_window_rect){
        MC_Rect2IU rect;
        MC_Error rect_status = v->get_window_rect(wm->target, window->target, &rect);
        if(rect_status == MCE_OK){
            window->cached.rect = rect;
            position->x = rect.x;
            position->y = rect.y;
            return MCE_OK;
        }

        status = rect_status;
    }

    return status;
}

static MC_Error owner_get_size(MC_WMWindow *window, MC_Size2U *size){
    MC_WM *wm = window->wm;
    MC_WMVtab *v = &wm->vtab;

    MC_Error status = MCE_NOT_SUPPORTED;

    if(v->get_window_size){
        MC_Error size_status = v->get_window_size(wm->target, window->target, size);
        if(size_status == MCE_OK){
            window->cached.rect.width = size->width;
            window->cached.rect.height = size->height;
            return MCE_OK;
        }

        status = size_status;
    }

    if(v->get_window_rect){
        MC_Rect2IU rect;
        MC_Error rect_status = v->get_window_rect(wm->target, window->target, &rect);
        if(rect_status == MCE_OK){
            window->cached.rect = rect;
            size->width = rect.width;
            size->height = rect.height;
            return MCE_OK;
        }

        status = rect_status;
    }

    return status;
}

static MC_Error owner_get_rect(MC_WMWindow *window, MC_Rect2IU *rect){
    MC_WM *wm = window->wm;
    MC_WMVtab *v = &wm->vtab;

    MC_Error status = MCE_NOT_SUPPORTED;

    if(v->get_window_rect){
        MC_Error rect_status = v->get_window_rect(wm->target, window->target, rect);
        if(rect_status == MCE_OK){
            window->cached.rect = *rect;
            return MCE_OK;
        }

        status = rect_status;
    }

    if(v->get_window_position){
        MC_Vec2i position;
        MC_Error position_status = v->get_window_position(wm->target, window->target, &position);
        if(position_status == MCE_OK){
            window->cached.rect.x = position.x;
            window->cached.rect.y = position.y;
        }

        status = position_status;
    }

    if(v->get_window_size){
        MC_Size2U size;
        MC_Error size_status = v->get_window_size(wm->target, window->target, &size);
        if(size_status == MCE_OK){
            window->cached.rect.width = size.width;
            window->cached.rect.height = size.height;
        }

        status = size_status;
    }

    return status;
}

static MC_Error foreign_set_title(MC_ForeignWindow *foreign, MC_Str title){
    MC_WM *wm = foreign->wm;

    if(wm->vtab.set_foreign_window_title){
        return wm->vtab.set_foreign_window_title(wm->target, foreign->target, title);
    }

    return MCE_NOT_SUPPORTED;
}

static MC_Error foreign_set_rect(MC_ForeignWindow *foreign, MC_Rect2IU rect){
    MC_WM *wm = foreign->wm;

    if(wm->vtab.set_foreign_window_rect){
        return wm->vtab.set_foreign_window_rect(wm->target, foreign->target, rect);
    }

    return MCE_NOT_SUPPORTED;
}

static MC_Error foreign_set_position(MC_ForeignWindow *foreign, MC_Vec2i position){
    MC_Rect2IU rect;
    MC_RETURN_ERROR(foreign_get_rect(foreign, &rect));

    rect.x = position.x;
    rect.y = position.y;

    return foreign_set_rect(foreign, rect);
}

static MC_Error foreign_set_size(MC_ForeignWindow *foreign, MC_Size2U size){
    MC_Rect2IU rect;
    MC_RETURN_ERROR(foreign_get_rect(foreign, &rect));

    rect.width = size.width;
    rect.height = size.height;

    return foreign_set_rect(foreign, rect);
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

static MC_Error foreign_get_rect(MC_ForeignWindow *foreign, MC_Rect2IU *rect){
    MC_WM *wm = foreign->wm;

    if(wm->vtab.get_foreign_window_rect){
        return wm->vtab.get_foreign_window_rect(wm->target, foreign->target, rect);
    }

    return MCE_NOT_SUPPORTED;
}

static MC_Error foreign_get_position(MC_ForeignWindow *foreign, MC_Vec2i *position){
    MC_Rect2IU rect;
    MC_RETURN_ERROR(foreign_get_rect(foreign, &rect));

    position->x = rect.x;
    position->y = rect.y;

    return MCE_OK;
}

static MC_Error foreign_get_size(MC_ForeignWindow *foreign, MC_Size2U *size){
    MC_Rect2IU rect;
    MC_RETURN_ERROR(foreign_get_rect(foreign, &rect));

    size->width = rect.width;
    size->height = rect.height;

    return MCE_OK;
}

MC_Error mc_wm_poll_event(MC_WM *wm, MC_WMEvent *event){
    MC_WMVtab *v = &wm->vtab;

    for(;;){
        while(wm->pending_indications){
            if(indication_category[wm->indications[0].type] & wm->events){
                *event = translate_indication(wm);
                return MCE_OK;
            }

            discard_indication(wm);
        }

        mc_arena_reset(wm->arena);

        if(v->heartbeat){
            v->heartbeat(wm->target);
        }

        if(!(v->poll_event && v->poll_event(wm->target, wm->target_event))){
            return MCE_AGAIN;
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
            *event = (MC_WMEvent){
                .type = MC_WME_RAW,
                .as.raw = wm->target_event
            };

            return MCE_OK;
        }
    }
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
    DUMP_IF_NOT_IMPLEMENTED(set_window_position);
    DUMP_IF_NOT_IMPLEMENTED(set_window_size);
    DUMP_IF_NOT_IMPLEMENTED(set_window_rect);
    DUMP_IF_NOT_IMPLEMENTED(set_window_state);
    DUMP_IF_NOT_IMPLEMENTED(get_window_title);
    DUMP_IF_NOT_IMPLEMENTED(get_window_position);
    DUMP_IF_NOT_IMPLEMENTED(get_window_size);
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
                .window = window,
            }
        };
    case MC_WMIND_WINDOW_MOVED:
        window = window_from_target(wm, ind.as.window_moved.window);

        return (MC_WMEvent){
            .type = MC_WME_WINDOW_MOVED,
            .as.window_moved = {
                .window = window,
                .new_position = ind.as.window_moved.new_position,
            }
        };
    case MC_WMIND_WINDOW_RESIZED:
        window = window_from_target(wm, ind.as.window_resized.window);

        return (MC_WMEvent){
            .type = MC_WME_WINDOW_RESIZED,
            .as.window_resized = {
                .window = window,
                .new_size = ind.as.window_resized.new_size,
            }
        };
    case MC_WMIND_WINDOW_REDRAW_REQUESTED:
        window = window_from_target(wm, ind.as.redraw_requested.window);

        return (MC_WMEvent){
            .type = MC_WME_WINDOW_REDRAW_REQUESTED,
            .as.redraw_requested = {
                .window = window,
            }
        };
    case MC_WMIND_WINDOW_CLOSE_REQUESTED:
        window = window_from_target(wm, ind.as.window_close_requested.window);

        return (MC_WMEvent){
            .type = MC_WME_WINDOW_CLOSE_REQUESTED,
            .as.window_close_requested = {
                .window = window,
            }
        };
    case MC_WMIND_WINDOW_STATE_CHANGED:
        window = window_from_target(wm, ind.as.window_state_changed.window);
        window->cached.state = ind.as.window_state_changed.state;

        return (MC_WMEvent){
            .type = MC_WME_WINDOW_STATE_CHANGED,
            .as.window_state_changed = {
                .window = window,
                .state = ind.as.window_state_changed.state,
            }
        };
    case MC_WMIND_FOCUS_GAINED:
        window = window_from_target(wm, ind.as.focus_gained.window);

        return (MC_WMEvent){
            .type = MC_WME_FOCUS_GAINED,
            .as.focus_gained = {
                .window = window,
            }
        };
    case MC_WMIND_FOCUS_LOST:
        window = window_from_target(wm, ind.as.focus_lost.window);

        return (MC_WMEvent){
            .type = MC_WME_FOCUS_LOST,
            .as.focus_lost = {
                .window = window,
            }
        };
    case MC_WMIND_MOUSE_DOWN:
        window = window_from_target(wm, ind.as.mouse_down.window);

        return (MC_WMEvent){
            .type = MC_WME_MOUSE_DOWN,
            .as.mouse_down = {
                .window = window,
                .button = ind.as.mouse_down.button,
                .position = window->cached.mouse_pos,
            }
        };
    case MC_WMIND_MOUSE_UP:
        window = window_from_target(wm, ind.as.mouse_up.window);

        return (MC_WMEvent){
            .type = MC_WME_MOUSE_UP,
            .as.mouse_up = {
                .window = window,
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
                .window = window,
                .position = ind.as.mouse_moved.position,
            }
        };
    case MC_WMIND_MOUSE_ENTER:
        window = window_from_target(wm, ind.as.mouse_enter.window);
        window->cached.mouse_over = true;

        return (MC_WMEvent){
            .type = MC_WME_MOUSE_ENTER,
            .as.mouse_enter = {
                .window = window,
                .position = window->cached.mouse_pos,
            }
        };
    case MC_WMIND_MOUSE_LEAVE:
        window = window_from_target(wm, ind.as.mouse_leave.window);
        window->cached.mouse_over = false;

        return (MC_WMEvent){
            .type = MC_WME_MOUSE_LEAVE,
            .as.mouse_enter = {
                .window = window,
                .position = window->cached.mouse_pos,
            }
        };
    case MC_WMIND_KEY_DOWN:
        return (MC_WMEvent){
            .type = MC_WME_KEY_DOWN,
            .as.key_down = {
                .window = NULL,
                .key = ind.as.key_down.key,
            }
        };
    case MC_WMIND_KEY_UP:
        return (MC_WMEvent){
            .type = MC_WME_KEY_UP,
            .as.key_up = {
                .window = NULL,
                .key = ind.as.key_up.key,
            }
        };
    case MC_WMIND_TEXT_INPUT:
        window = window_from_target(wm, ind.as.text_input.window);

        {
            MC_WMEvent event = {
                .type = MC_WME_TEXT_INPUT,
                .as.text_input = {
                    .window = window,
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
                .window = window,
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

static MC_ForeignWindow *find_foreign(MC_WM *wm, uint64_t identity){
    MC_ForeignWindow **w;
    MC_VECTOR_EACH(wm->foreign_windows, w){
        if((*w)->identity == identity){
            return *w;
        }
    }

    return NULL;
}
