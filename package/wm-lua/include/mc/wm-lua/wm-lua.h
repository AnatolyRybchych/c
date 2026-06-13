#ifndef MC_WM_LUA_H
#define MC_WM_LUA_H

#include <mc/wm/wm.h>
#include <mc/data/json.h>

struct lua_State;

void mc_wm_lua_open(struct lua_State *L, const MC_WMVtab *vtab);

int mc_wm_lua_module(struct lua_State *L, const MC_WMVtab *vtab);

void mc_wm_lua_push_window(struct lua_State *L, MC_WindowRef *window);

MC_WindowRef *mc_wm_lua_check_window(struct lua_State *L, int idx);

void mc_wm_lua_push_json(struct lua_State *L, MC_Json *json);

#endif // MC_WM_LUA_H
