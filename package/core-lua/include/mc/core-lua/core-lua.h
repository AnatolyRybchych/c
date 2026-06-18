#ifndef MC_CORE_LUA_H
#define MC_CORE_LUA_H

#include <lua.h>

#include <mc/sched.h>

int mc_core_lua_module(lua_State *L);
void mc_core_lua_open(lua_State *L);

int mc_core_lua_push_sched(lua_State *L, MC_Sched *sched);
MC_Sched *mc_core_lua_pop_sched(lua_State *L, int ref);

#endif // MC_CORE_LUA_H
