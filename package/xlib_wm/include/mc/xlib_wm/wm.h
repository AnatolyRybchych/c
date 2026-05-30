#ifndef MC_XLIBWM_H
#define MC_XLIBWM_H

#include <mc/wm/target.h>

#include <X11/Xlib.h>

extern const MC_WMVtab *mc_xlib_wm_vtab;

Display *mc_wm_xlib_get_display(MC_TargetWM *wm);
Window mc_wm_xlib_window_get_xid(MC_TargetWMWindow *window);

#endif // MC_XLIBWM_H
