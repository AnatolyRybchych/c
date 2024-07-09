#ifndef MC_GRAPHICS_XLIBGC_H
#define MC_GRAPHICS_XLIBGC_H

#include <mc/graphics/target.h>

#include <X11/Xlib.h>

MC_Error mc_xlib_graphics_init(MC_Graphics **g, Display *dpy, Drawable drawable);

#endif // MC_GRAPHICS_XLIBGC_H
