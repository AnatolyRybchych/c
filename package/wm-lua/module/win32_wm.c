#include <mc/wm-lua/wm-lua.h>
#include <mc/win32_wm/wm.h>

#include <lua.h>

int luaopen_mcwm(lua_State *L){
    return mc_wm_lua_module(L, mc_win32_wm_vtab);
}
