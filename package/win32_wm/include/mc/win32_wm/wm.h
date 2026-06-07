#ifndef MC_WIN32WM_H
#define MC_WIN32WM_H

#include <mc/wm/target.h>

#include <windows.h>

extern const MC_WMVtab *mc_win32_wm_vtab;

HWND mc_wm_win32_window_get_hwnd(MC_TargetWMWindow *window);

#endif // MC_WIN32WM_H
