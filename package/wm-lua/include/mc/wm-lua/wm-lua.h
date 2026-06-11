#ifndef MC_WM_LUA_H
#define MC_WM_LUA_H

#include <mc/wm/wm.h>

struct lua_State;

void mc_wm_lua_open(struct lua_State *L, const MC_WMVtab *vtab);

#endif // MC_WM_LUA_H
