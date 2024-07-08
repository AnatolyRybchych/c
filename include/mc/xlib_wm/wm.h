#ifndef MC_XLIBWM_H
#define MC_XLIBWM_H

#include <mc/wm/target.h>

#include <X11/Xlib.h>

extern const MC_WMVtab *mc_xlib_wm_vtab;

struct MC_TargetWM{
    MC_Stream *log;
    Display *dpy;
};

struct MC_TargetWMWindow{
    Window window_id;
};

struct MC_TargetWMEvent{
    XEvent event;
};


#endif // MC_XLIBWM_H
