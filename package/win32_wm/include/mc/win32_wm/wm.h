#ifndef MC_WIN32WM_H
#define MC_WIN32WM_H

#include <mc/wm/target.h>
#include <mc/wm/key.h>

#include <windows.h>

extern const MC_WMVtab *mc_win32_wm_vtab;

HWND mc_wm_win32_window_get_hwnd(MC_TargetWMWindow *window);

MC_Error mc_wm_win32_identity_from_hwnd(MC_TargetWM *wm, HWND hwnd, uint64_t *identity);

void mc_wm_win32_set_keyboard_suppress(MC_TargetWM *wm,
    bool (*suppress)(MC_TargetWM *wm, MC_Key key, bool down));

void mc_wm_win32_set_mouse_suppress(MC_TargetWM *wm,
    bool (*suppress)(MC_TargetWM *wm, UINT message, int x, int y));

#endif
