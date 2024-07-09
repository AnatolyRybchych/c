#ifndef MC_WM_WM_H
#define MC_WM_WM_H

#include <mc/io/stream.h>
#include <mc/error.h>
#include <mc/geometry.h>
#include <mc/data/str.h>

#include <stddef.h>
#include <stdbool.h>

typedef struct MC_WM MC_WM;
typedef struct MC_WMWindow MC_WMWindow;
typedef struct MC_WMVtab MC_WMVtab;
typedef struct MC_WMEvent MC_WMEvent;

MC_Error mc_wm_init(MC_WM **wm, const MC_WMVtab *vtab);
void mc_wm_destroy(MC_WM *wm);

MC_Error mc_wm_window_init(MC_WM *wm, MC_WMWindow **window);
void mc_wm_window_destroy(MC_WMWindow *window);

MC_Error mc_wm_window_set_title(MC_WMWindow *window, MC_Str title);
MC_Error mc_wm_window_set_position(MC_WMWindow *window, MC_Point2I position);
MC_Error mc_wm_window_set_size(MC_WMWindow *window, MC_Size2U size);
MC_Error mc_wm_window_set_rect(MC_WMWindow *window, MC_Rect2IU rect);

MC_Error mc_wm_window_get_title(MC_WMWindow *window, MC_Str *title);
MC_Error mc_wm_window_get_position(MC_WMWindow *window, MC_Point2I *position);
MC_Error mc_wm_window_get_size(MC_WMWindow *window, MC_Size2U *size);
MC_Error mc_wm_window_get_rect(MC_WMWindow *window, MC_Rect2IU *rect);

MC_Str mc_wm_window_cached_get_title(MC_WMWindow *window);
MC_Point2I mc_wm_window_cached_get_position(MC_WMWindow *window);
MC_Size2U mc_wm_window_cached_get_size(MC_WMWindow *window);
MC_Rect2IU mc_wm_window_cached_get_rect(MC_WMWindow *window);
bool mc_wm_window_cached_is_mouse_over(MC_WMWindow *window);

MC_Error mc_wm_poll_event(MC_WM *wm, MC_WMEvent *event);

#endif // MC_WM_WM_H
