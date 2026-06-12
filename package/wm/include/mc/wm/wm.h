#ifndef MC_WM_WM_H
#define MC_WM_WM_H

#include <mc/data/stream.h>
#include <mc/error.h>
#include <mc/geometry/size.h>
#include <mc/geometry/lina.h>
#include <mc/geometry/rect.h>
#include <mc/data/str.h>

#include <stddef.h>
#include <stdbool.h>

typedef struct MC_WM MC_WM;
typedef struct MC_WMWindow MC_WMWindow;
typedef struct MC_WMVtab MC_WMVtab;
typedef struct MC_WMEvent MC_WMEvent;

typedef struct MC_WindowRef MC_WindowRef;
typedef struct MC_ForeignWindow MC_ForeignWindow;

typedef struct MC_TargetWM MC_TargetWM;
typedef struct MC_TargetWMWindow MC_TargetWMWindow;
typedef struct MC_TargetForeignWindow MC_TargetForeignWindow;

#define MC_WM_TEXT_INPUT_CAP 32

typedef unsigned MC_WMWindowState;
enum MC_WMWindowState{
    MC_WM_WINDOW_STATE_NORMAL,
    MC_WM_WINDOW_STATE_MINIMIZED,
    MC_WM_WINDOW_STATE_MAXIMIZED,
    MC_WM_WINDOW_STATE_FULLSCREEN,
};

typedef unsigned MC_WMEvents;
enum MC_WMEvents{
    MC_WM_EVENTS_NONE            = 0,

    MC_WM_EVENTS_RAW             = 1 << 0,

    MC_WM_EVENTS_CORE            = 1 << 1,
    MC_WM_EVENTS_WINDOW_CORE     = 1 << 2,
    MC_WM_EVENTS_GLOBAL_KEYBOARD = 1 << 3,
    MC_WM_EVENTS_GLOBAL_MOUSE    = 1 << 4,
};

typedef enum MC_WMArea {
    MC_WM_AREA_WINDOW = 0,
    MC_WM_AREA_DECORATED = 1,
    MC_WM_AREA_DRAWABLE = 2,
    MC_WM_AREA_COUNT = 3,
} MC_WMArea;

struct MC_Graphics;

MC_Error mc_wm_init(MC_WM **wm, const MC_WMVtab *vtab);
MC_WM *mc_wm_ref(MC_WM *wm);
void mc_wm_destroy(MC_WM *wm);
const char *mc_wm_impl_name(MC_WM *wm);
struct MC_TargetWM *mc_wm_get_target(MC_WM *wm);

MC_Error mc_wm_request_events(MC_WM *wm, MC_WMEvents events);
MC_WMEvents mc_wm_get_requested_events(MC_WM *wm);

MC_Error mc_wm_window_init(MC_WM *wm, MC_WMWindow **window);
void mc_wm_window_destroy(MC_WMWindow *window);
MC_WindowRef *mc_wm_window_get_ref(MC_WMWindow *window);
struct MC_TargetWMWindow *mc_wm_window_get_target(MC_WMWindow *window);
MC_Error mc_wm_window_get_graphic(MC_WMWindow *window, struct MC_Graphics **g);

MC_Str mc_wm_window_cached_get_title(MC_WMWindow *window);
MC_Vec2i mc_wm_window_cached_get_position(MC_WMWindow *window, MC_WMArea area);
MC_Size2U mc_wm_window_cached_get_size(MC_WMWindow *window, MC_WMArea area);
MC_Rect2IU mc_wm_window_cached_get_rect(MC_WMWindow *window, MC_WMArea area);
MC_WMWindowState mc_wm_window_cached_get_state(MC_WMWindow *window);
bool mc_wm_window_cached_is_mouse_over(MC_WMWindow *window);

MC_Error mc_wm_get_focused_window(MC_WM *wm, MC_WindowRef **window);
MC_Error mc_wm_get_hovered_window(MC_WM *wm, MC_WindowRef **window);
MC_Error mc_wm_get_all_windows(MC_WM *wm, MC_Error (*visit)(MC_WindowRef *window, void *ctx), void *ctx);
MC_Error mc_wm_resolve_window(MC_WM *wm, uint64_t identity, MC_WindowRef **window);
struct MC_TargetForeignWindow *mc_wm_window_get_foreign_target(MC_WindowRef *window);
MC_Error mc_wm_window_get_identity(MC_WindowRef *window, uint64_t *out);

MC_WindowRef *mc_wm_window_ref(MC_WindowRef *window);
void mc_wm_window_unref(MC_WindowRef *window);
unsigned mc_wm_window_refcount(MC_WindowRef *window);
bool mc_wm_window_is_alive(MC_WindowRef *window);

MC_Error mc_wm_window_close(MC_WindowRef *window);

MC_Error mc_wm_window_set_title(MC_WindowRef *window, MC_Str title);
MC_Error mc_wm_window_set_state(MC_WindowRef *window, MC_WMWindowState state);
MC_Error mc_wm_window_get_title(MC_WindowRef *window, char *utf8, size_t cap, size_t *len);
MC_Error mc_wm_window_get_state(MC_WindowRef *window, MC_WMWindowState *state);

MC_Error mc_wm_window_set_position(MC_WindowRef *window, MC_WMArea area, MC_Vec2i position);
MC_Error mc_wm_window_set_size(MC_WindowRef *window, MC_WMArea area, MC_Size2U size);
MC_Error mc_wm_window_set_rect(MC_WindowRef *window, MC_WMArea area, MC_Rect2IU rect);
MC_Error mc_wm_window_get_position(MC_WindowRef *window, MC_WMArea area, MC_Vec2i *position);
MC_Error mc_wm_window_get_size(MC_WindowRef *window, MC_WMArea area, MC_Size2U *size);
MC_Error mc_wm_window_get_rect(MC_WindowRef *window, MC_WMArea area, MC_Rect2IU *rect);

MC_Error mc_wm_poll_event(MC_WM *wm, MC_WMEvent *event);

#endif // MC_WM_WM_H
