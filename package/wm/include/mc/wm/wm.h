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

typedef struct MC_TargetWM MC_TargetWM;
typedef struct MC_TargetWMWindow MC_TargetWMWindow;

struct MC_Graphics;

MC_Error mc_wm_init(MC_WM **wm, const MC_WMVtab *vtab);
void mc_wm_destroy(MC_WM *wm);
struct MC_TargetWM *mc_wm_get_target(MC_WM *wm);

MC_Error mc_wm_window_init(MC_WM *wm, MC_WMWindow **window);
void mc_wm_window_destroy(MC_WMWindow *window);
struct MC_TargetWMWindow *mc_wm_window_get_target(MC_WMWindow *wm);

MC_Error mc_wm_window_set_title(MC_WMWindow *window, MC_Str title);
MC_Error mc_wm_window_set_position(MC_WMWindow *window, MC_Vec2i position);
MC_Error mc_wm_window_set_size(MC_WMWindow *window, MC_Size2U size);
MC_Error mc_wm_window_set_rect(MC_WMWindow *window, MC_Rect2IU rect);

MC_Error mc_wm_window_get_title(MC_WMWindow *window, MC_Str *title);
MC_Error mc_wm_window_get_position(MC_WMWindow *window, MC_Vec2i *position);
MC_Error mc_wm_window_get_size(MC_WMWindow *window, MC_Size2U *size);
MC_Error mc_wm_window_get_rect(MC_WMWindow *window, MC_Rect2IU *rect);

MC_Error mc_wm_window_get_graphic(MC_WMWindow *window, struct MC_Graphics **g);

MC_Str mc_wm_window_cached_get_title(MC_WMWindow *window);
MC_Vec2i mc_wm_window_cached_get_position(MC_WMWindow *window);
MC_Size2U mc_wm_window_cached_get_size(MC_WMWindow *window);
MC_Rect2IU mc_wm_window_cached_get_rect(MC_WMWindow *window);
bool mc_wm_window_cached_is_mouse_over(MC_WMWindow *window);

MC_Error mc_wm_poll_event(MC_WM *wm, MC_WMEvent *event);

#endif // MC_WM_WM_H
