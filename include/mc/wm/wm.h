#ifndef MC_WM_WM_H
#define MC_WM_WM_H

#include <mc/io/stream.h>
#include <mc/error.h>

#include <stddef.h>
#include <stdbool.h>

#define MC_WM_MAX_EVENTS_PER_TARGET_EVENT 16

#define MC_ITER_WM_EVENTS() \
    MC_EVENT(NONE) \
    MC_EVENT(RAW) \

typedef struct MC_WM MC_WM;
typedef struct MC_WMWindow MC_WMWindow;
typedef struct MC_WMVtab MC_WMVtab;
typedef struct MC_WMEvent MC_WMEvent;

typedef unsigned MC_WMEventType;
enum MC_WMEventType{
#define MC_EVENT(NAME, ...) MC_WME_##NAME,
    MC_ITER_WM_EVENTS()
#undef MC_EVENT

    MC_WME_COUNT
};

MC_Error mc_wm_init(MC_WM **wm, const MC_WMVtab *vtab);
void mc_wm_destroy(MC_WM *wm);

MC_Error mc_wm_init_window(MC_WM *wm, MC_WMWindow **window);
void mc_wm_destroy_window(MC_WMWindow *window);

MC_Error mc_wm_poll_event(MC_WM *wm, MC_WMEvent *event);

struct MC_TargetWM;
struct MC_TargetWMWindow;
struct MC_TargetWMEvent;

struct MC_WMEvent{
    MC_WMEventType type;
    union {
        void *raw;
    } as;
};

struct MC_WMVtab{
    char name[32];

    size_t wm_size;
    size_t window_size;
    size_t event_size;

    MC_Error (*init)(struct MC_TargetWM *wm, MC_Stream *log);
    void (*destroy)(struct MC_TargetWM *wm);

    MC_Error (*init_window)(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window);
    void (*destroy_window)(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window);

    bool (*poll_event)(struct MC_TargetWM *wm, struct MC_TargetWMEvent *event);
    unsigned (*translate_event)(struct MC_TargetWM *wm, const struct MC_TargetWMEvent *event, MC_WMEvent events[MC_WM_MAX_EVENTS_PER_TARGET_EVENT]);
};

#endif // MC_WM_WM_H
