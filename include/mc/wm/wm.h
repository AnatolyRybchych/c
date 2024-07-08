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

MC_Error mc_wm_init_window(MC_WM *wm, MC_WMWindow **window);
void mc_wm_destroy_window(MC_WMWindow *window);

MC_Error mc_wm_set_window_title(MC_WMWindow *window, MC_Str title);
MC_Error mc_wm_set_window_position(MC_WMWindow *window, MC_Point2I position);
MC_Error mc_wm_set_window_size(MC_WMWindow *window, MC_Size2U size);
MC_Error mc_wm_set_window_rect(MC_WMWindow *window, MC_Rect2IU rect);

MC_Error mc_wm_get_window_title(MC_WMWindow *window, MC_Str *title);
MC_Error mc_wm_get_window_position(MC_WMWindow *window, MC_Point2I *position);
MC_Error mc_wm_get_window_size(MC_WMWindow *window, MC_Size2U *size);
MC_Error mc_wm_get_window_rect(MC_WMWindow *window, MC_Rect2IU *rect);

MC_Error mc_wm_poll_event(MC_WM *wm, MC_WMEvent *event);

#endif // MC_WM_WM_H
