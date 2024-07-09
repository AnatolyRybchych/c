#include <mc/wm/wm.h>
#include <mc/wm/target.h>
#include <mc/wm/event.h>

#include <mc/data/string.h>
#include <mc/data/vector.h>

#include <mc/io/file.h>
#include <mc/util/assert.h>
#include <mc/util/memory.h>

#include <malloc.h>
#include <memory.h>

#define INDICATIONS_BUFFER_SIZE (MC_WM_MAX_INDICATIONS_PER_EVENT * 16)

#define WM_LOGE(FMT, ...) if(wm->log) mc_fmt(wm->log, "[WM] " FMT "\n", ##__VA_ARGS__)
#define WM_LOG_DEV(FMT, ...) if(wm->log) mc_fmt(wm->log, "%s() [DEV][WM][%s] " FMT "\n", __func__, wm->vtab.name, ##__VA_ARGS__)

MC_DEFINE_VECTOR(Windows, MC_WMWindow*);

typedef struct indicationNode indicationNode;

struct MC_WMWindow{
    MC_WM *wm;
    struct MC_TargetWMWindow *target;
    MC_Rect2IU cached_rect;
    MC_Stream *tmp_stream;
    MC_String *cached_title;
    uint8_t data[];
};

struct MC_WM{
    MC_WMVtab vtab;
    MC_Stream *log;
    struct MC_TargetWM *target;
    struct MC_TargetWMEvent *target_event;

    unsigned pending_indications;
    MC_TargetIndication indications[INDICATIONS_BUFFER_SIZE];

    Windows *windows;

    alignas(void*) uint8_t data[];
};

static void dump_vtable(MC_WM *wm);
static void handle_duplicate_indications(MC_WM *wm);
static MC_WMEvent translate_indication(MC_WM *wm);
static MC_WMWindow *window_from_target(MC_WM *wm, struct MC_TargetWMWindow *target);

MC_Error mc_wm_init(MC_WM **ret_wm, const MC_WMVtab *vtab){
    if(!vtab || !vtab->init){
        return MCE_NOT_SUPPORTED;
    }

    MC_WM *wm;
    struct MC_TargetWMEvent *target_event;

    MC_Error allocation_status = mc_malloc_all(
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

    MC_Error init_status = vtab->init(wm->target, wm->log);
    if(init_status != MCE_OK){
        free(wm->target_event);
        free(wm);
        return init_status;
    }

    *ret_wm = wm;
    dump_vtable(wm);

    return MCE_OK;
}

void mc_wm_destroy(MC_WM *wm){
    if(wm->vtab.destroy){
        wm->vtab.destroy(wm->target);
    }

    while(!MC_VECTOR_EMPTY(wm->windows)){
        mc_wm_destroy_window(wm->windows->beg[0]);
    }

    MC_VECTOR_FREE(wm->windows);

    free(wm);
}

MC_Error mc_wm_init_window(MC_WM *wm, MC_WMWindow **ret_window){
    *ret_window = NULL;
    MC_WMVtab *v = &wm->vtab;

    if(v->init_window == NULL){
        return MCE_NOT_SUPPORTED;
    }

    MC_WMWindow *window = malloc(sizeof(MC_WMWindow) + sizeof(v->window_size));
    if(window == NULL){
        return MCE_OUT_OF_MEMORY;
    }

    Windows *new_windows = MC_VECTOR_PUSHN(wm->windows, 1, &window);
    if(new_windows == NULL){
        free(window);
        return MCE_OUT_OF_MEMORY;
    }

    window->target = (struct MC_TargetWMWindow*)window->data;
    MC_Error status = v->init_window(wm->target, window->target);
    if(status != MCE_OK){
        wm->windows->end--;
        free(window);
        return MCE_OUT_OF_MEMORY;
    }

    *ret_window = window;
    return MCE_OK;
}

void mc_wm_destroy_window(MC_WMWindow *window){
    MC_WM *wm = window->wm;
    MC_WMVtab *v = &wm->vtab;

    if(v->destroy_window){
        v->destroy_window(wm->target, window->target);
    }

    free(window);

    size_t idx = 0;
    MC_WMWindow **w;
    MC_VECTOR_EACH(wm->windows, w){
        if(*w == window){
            break;
        }

        idx++;
    }

    MC_VECTOR_ERASE(wm->windows, idx, 1);
}

MC_Error mc_wm_set_window_title(MC_WMWindow *window, MC_Str title){
    MC_WM *wm = window->wm;
    MC_WMVtab *v = &wm->vtab;

    if(v->set_window_title){
        return v->set_window_title(wm->target, window->target, title);
    }

    return MCE_NOT_SUPPORTED;
}

MC_Error mc_wm_set_window_position(MC_WMWindow *window, MC_Point2I position){
    MC_WM *wm = window->wm;
    MC_WMVtab *v = &wm->vtab;

    MC_Error status = MCE_NOT_SUPPORTED;

    if(v->set_window_position){
        status = v->set_window_position(wm->target, window->target, position);
        if(status == MCE_OK){
            window->cached_rect.x = position.x;
            window->cached_rect.y = position.y;
            return MCE_OK;
        }
    }

    if(v->set_window_rect){
        status = v->set_window_rect(wm->target, window->target, (MC_Rect2IU){
            .x = position.x,
            .y = position.y,
            .width = window->cached_rect.width,
            .height = window->cached_rect.height
        });

        if(status == MCE_OK){
            window->cached_rect.x = position.x;
            window->cached_rect.y = position.y;
            return MCE_OK;
        }
    }

    return status;
}

MC_Error mc_wm_set_window_size(MC_WMWindow *window, MC_Size2U size){
    MC_WM *wm = window->wm;
    MC_WMVtab *v = &wm->vtab;

    MC_Error status = MCE_NOT_SUPPORTED;

    if(v->set_window_size){
        status = v->set_window_size(wm->target, window->target, size);
        if(status == MCE_OK){
            window->cached_rect.width = size.width;
            window->cached_rect.height = size.height;
            return MCE_OK;
        }
    }

    if(v->set_window_rect){
        status = v->set_window_rect(wm->target, window->target, (MC_Rect2IU){
            .x = window->cached_rect.x,
            .y = window->cached_rect.y,
            .width = size.width,
            .height = size.height
        });

        if(status == MCE_OK){
            window->cached_rect.width = size.width;
            window->cached_rect.height = size.height;
            return MCE_OK;
        }
    }

    return status;
}

MC_Error mc_wm_set_window_rect(MC_WMWindow *window, MC_Rect2IU rect){
    MC_WM *wm = window->wm;
    MC_WMVtab *v = &wm->vtab;

    MC_Error status = MCE_NOT_SUPPORTED;

    if(v->set_window_rect){
        status = v->set_window_rect(wm->target, window->target, rect);
        if(status == MCE_OK){
            window->cached_rect = rect;
            return MCE_OK;
        }
    }

    if(v->set_window_position && v->set_window_size){
        MC_Error position_status = v->set_window_position(wm->target, window->target, (MC_Point2I){
            .x = rect.x,
            .y = rect.y,
        });

        MC_Error size_status = v->set_window_size(wm->target, window->target, (MC_Size2U){
            .width = rect.width,
            .height = rect.height,
        });

        if(position_status == MCE_OK){
            window->cached_rect.x = rect.x;
            window->cached_rect.y = rect.y;
        }

        if(size_status == MCE_OK){
            window->cached_rect.width = rect.width;
            window->cached_rect.height = rect.height;
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


MC_Error mc_wm_get_window_title(MC_WMWindow *window, MC_Str *title){
    MC_WM *wm = window->wm;
    MC_WMVtab *v = &wm->vtab;

    MC_Error status = MCE_NOT_SUPPORTED;

    if(v->get_window_title){
        status = v->get_window_title(wm->target, window->target, window->tmp_stream);
        if(status == MCE_OK){
            // window->cached_title = mc_read_to_end(window->tmp_stream);
            if(window->cached_title == NULL){
                return MCE_OUT_OF_MEMORY;
            }

            *title = mc_string_str(window->cached_title);
        }

        return status;
    }

    return status;
}

MC_Error mc_wm_get_window_position(MC_WMWindow *window, MC_Point2I *position){
    MC_WM *wm = window->wm;
    MC_WMVtab *v = &wm->vtab;

    MC_Error status = MCE_NOT_SUPPORTED;

    if(v->get_window_position){
        MC_Error position_status = v->get_window_position(wm->target, window->target, position);
        if(position_status == MCE_OK){
            window->cached_rect.x = position->x;
            window->cached_rect.y = position->y;
            return MCE_OK;
        }

        status = position_status;
    }

    if(v->get_window_rect){
        MC_Rect2IU rect;
        MC_Error rect_status = v->get_window_rect(wm->target, window->target, &rect);
        if(rect_status == MCE_OK){
            window->cached_rect = rect;
            position->x = rect.x;
            position->y = rect.y;
            return MCE_OK;
        }

        status = rect_status;
    }

    return status;
}

MC_Error mc_wm_get_window_size(MC_WMWindow *window, MC_Size2U *size){
    MC_WM *wm = window->wm;
    MC_WMVtab *v = &wm->vtab;

    MC_Error status = MCE_NOT_SUPPORTED;

    if(v->get_window_size){
        MC_Error size_status = v->get_window_size(wm->target, window->target, size);
        if(size_status == MCE_OK){
            window->cached_rect.width = size->width;
            window->cached_rect.height = size->height;
            return MCE_OK;
        }

        status = size_status;
    }

    if(v->get_window_rect){
        MC_Rect2IU rect;
        MC_Error rect_status = v->get_window_rect(wm->target, window->target, &rect);
        if(rect_status == MCE_OK){
            window->cached_rect = rect;
            size->width = rect.width;
            size->height = rect.height;
            return MCE_OK;
        }

        status = rect_status;
    }

    return status;
}

MC_Error mc_wm_get_window_rect(MC_WMWindow *window, MC_Rect2IU *rect){
    MC_WM *wm = window->wm;
    MC_WMVtab *v = &wm->vtab;

    MC_Error status = MCE_NOT_SUPPORTED;

    if(v->get_window_rect){
        MC_Error rect_status = v->get_window_rect(wm->target, window->target, rect);
        if(rect_status == MCE_OK){
            window->cached_rect = *rect;
            return MCE_OK;
        }

        status = rect_status;
    }

    if(v->get_window_position){
        MC_Point2I position;
        MC_Error position_status = v->get_window_position(wm->target, window->target, &position);
        if(position_status == MCE_OK){
            window->cached_rect.x = position.x;
            window->cached_rect.y = position.y;
        }

        status = position_status;
    }

    if(v->get_window_size){
        MC_Size2U size;
        MC_Error size_status = v->get_window_size(wm->target, window->target, &size);
        if(size_status == MCE_OK){
            window->cached_rect.width = size.width;
            window->cached_rect.height = size.height;
        }

        status = size_status;
    }

    return status;
}

MC_Error mc_wm_poll_event(MC_WM *wm, MC_WMEvent *event){
    MC_WMVtab *v = &wm->vtab;

    if(wm->pending_indications < INDICATIONS_BUFFER_SIZE - MC_WM_MAX_INDICATIONS_PER_EVENT){
        if(v->poll_event && v->poll_event(wm->target, wm->target_event)){
            *event = (MC_WMEvent){
                .type = MC_WME_RAW,
                .as.raw = wm->target_event
            };

            if(v->translate_event){
                unsigned new_indications = v->translate_event(
                    wm->target, wm->target_event,
                    wm->indications + wm->pending_indications);

                MC_ASSERT_FAULT(new_indications <= MC_WM_MAX_INDICATIONS_PER_EVENT);

                wm->pending_indications += new_indications;
            }

            return MCE_OK;
        }
    }

    if(wm->pending_indications){
        *event = translate_indication(wm);
        return MCE_OK;
    }
    else{
        return MCE_AGAIN;
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
        #define MC_WMIDN(NAME, DUP_ACTION) [MC_WMIND_##NAME] = MC_WM_INDDUP_##DUP_ACTION,
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

static MC_WMEvent translate_indication(MC_WM *wm){
    handle_duplicate_indications(wm);

    MC_TargetIndication ind = *wm->indications;
    memmove(wm->indications, wm->indications + 1, sizeof(MC_TargetIndication[--wm->pending_indications]));
    
    switch (ind.type){
    case MC_WMIND_WINDOW_READY:
        return (MC_WMEvent){
            .type = MC_WME_WINDOW_READY,
            .as.window_ready = {
                .window = window_from_target(wm, ind.as.window_ready.window),
            }
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
