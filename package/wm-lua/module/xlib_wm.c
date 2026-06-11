#include <mc/wm-lua/wm-lua.h>
#include <mc/xlib_wm/wm.h>

#include <lua.h>

int luaopen_mcwm(lua_State *L){
    return mc_wm_lua_module(L, mc_xlib_wm_vtab);
}
