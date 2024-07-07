#include <mc/wm/wm.h>
#include <mc/io/file.h>
#include <mc/util/assert.h>
#include <mc/util/memory.h>

#include <malloc.h>
#include <memory.h>

#define WM_LOG(FMT, ...) if(wm->log) mc_fmt(wm->log, "[WM] " FMT "\n", ##__VA_ARGS__)

struct MC_WMWindow{
    MC_WM *wm;
    struct MC_TargetWMWindow *target;
    uint8_t data[];
};

struct MC_WM{
    MC_WMVtab vtab;
    MC_Stream *log;
    struct MC_TargetWM *target;
    struct MC_TargetWMEvent *last_target_event;

    unsigned pending_events;
    MC_WMEvent events[MC_WM_MAX_EVENTS_PER_TARGET_EVENT];

    uint8_t data[];
};

MC_Error mc_wm_init(MC_WM **ret_wm, const MC_WMVtab *vtab){
    *ret_wm = NULL;

    if(!vtab || !vtab->init){
        return MCE_NOT_SUPPORTED;
    }

    MC_WM *wm = malloc(
        sizeof(MC_WM) + 
        MC_ALIGN(sizeof(void*), vtab->wm_size) +
        MC_ALIGN(sizeof(void*), vtab->event_size));

    if(wm == NULL){
        return MCE_OUT_OF_MEMORY;
    }

    memset(wm, 0, sizeof(MC_WM) + vtab->wm_size);

    wm->target = MC_NEXT_AFTER(wm->data, 0, alignof(void*));
    wm->last_target_event = MC_NEXT_FIELD_ADDR(wm->target, alignof(void*));

    wm->log = MC_STDERR;

    MC_Error status = vtab->init(wm->target, wm->log);
    if(status != MCE_OK){
        free(wm);
        return status;
    }

    memcpy(&wm->vtab, vtab, sizeof(MC_WMVtab));

    *ret_wm = wm;

    return MCE_OK;
}

void mc_wm_destroy(MC_WM *wm){
    if(wm->vtab.destroy){
        wm->vtab.destroy(wm->target);
    }

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

    window->target = (struct MC_TargetWMWindow*)window->data;
    MC_Error status = v->init_window(wm->target, window->target);
    if(status != MCE_OK){
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
}

MC_Error mc_wm_poll_event(MC_WM *wm, MC_WMEvent *event){
    MC_WMVtab *v = &wm->vtab;

    if(wm->pending_events){
        *event = wm->events[wm->pending_events--];
        return MCE_OK;
    }

    if(!v->poll_event || !v->poll_event(wm->target, wm->last_target_event)){
        return MCE_AGAIN;
    }

    *event = (MC_WMEvent){
        .type = MC_WME_RAW,
        .as.raw = wm->last_target_event
    };

    if(v->translate_event){
        wm->pending_events = v->translate_event(wm->target, wm->last_target_event, wm->events);
        MC_ASSERT_FAULT(wm->pending_events <= MC_WM_MAX_EVENTS_PER_TARGET_EVENT);
    }

    return MCE_OK;
}
