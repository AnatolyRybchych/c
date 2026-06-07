#ifndef MC_WIN32WM_GRAPHICS_H
#define MC_WIN32WM_GRAPHICS_H

#include <mc/graphics/target.h>

#include <windows.h>

MC_Error mc_win32_graphics_init(MC_Graphics **g, HWND hwnd);

#endif // MC_WIN32WM_GRAPHICS_H
