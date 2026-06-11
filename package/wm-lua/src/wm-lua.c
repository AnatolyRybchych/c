#include <mc/wm-lua/wm-lua.h>

#include <lua.h>
#include <lauxlib.h>

#include <mc/wm/wm.h>
#include <mc/wm/event.h>
#include <mc/wm/resolver.h>
#include <mc/data/str.h>

#include <string.h>

#define WM_MT     "mc.wm.WM"
#define WINDOW_MT "mc.wm.Window"

typedef struct LuaWM{
    MC_WM *wm;
} LuaWM;

typedef struct LuaWindow{
    MC_WindowRef *ref;
    MC_WMWindow *window;
} LuaWindow;

static void require_ok(lua_State *L, MC_Error e, const char *what){
    if(e != MCE_OK){
        luaL_error(L, "mc.wm: %s failed (error %d)", what, (int)e);
    }
}

static lua_Integer field_int(lua_State *L, int table, const char *key){
    lua_getfield(L, table, key);
    if(!lua_isnumber(L, -1)){
        luaL_error(L, "mc.wm: field '%s' must be a number", key);
    }

    lua_Integer value = lua_tointeger(L, -1);
    lua_pop(L, 1);
    return value;
}

static lua_Integer field_int_or(lua_State *L, int table, const char *key, lua_Integer fallback){
    lua_getfield(L, table, key);
    lua_Integer value = lua_isnil(L, -1) ? fallback : luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    return value;
}

static MC_Size2U check_size(lua_State *L, int table){
    luaL_checktype(L, table, LUA_TTABLE);
    return (MC_Size2U){
        .width = (unsigned)field_int(L, table, "width"),
        .height = (unsigned)field_int(L, table, "height"),
    };
}

static MC_Vec2i check_position(lua_State *L, int table){
    luaL_checktype(L, table, LUA_TTABLE);
    return (MC_Vec2i){
        .x = (int)field_int(L, table, "x"),
        .y = (int)field_int(L, table, "y"),
    };
}

static MC_Rect2IU check_rect(lua_State *L, int table){
    luaL_checktype(L, table, LUA_TTABLE);
    return (MC_Rect2IU){
        .x = (int)field_int_or(L, table, "x", 0),
        .y = (int)field_int_or(L, table, "y", 0),
        .width = (unsigned)field_int(L, table, "width"),
        .height = (unsigned)field_int(L, table, "height"),
    };
}

static MC_WMWindowState check_state(lua_State *L, int idx){
    const char *s = luaL_checkstring(L, idx);
    if(strcmp(s, "normal") == 0){
        return MC_WM_WINDOW_STATE_NORMAL;
    }
    if(strcmp(s, "minimized") == 0){
        return MC_WM_WINDOW_STATE_MINIMIZED;
    }
    if(strcmp(s, "maximized") == 0){
        return MC_WM_WINDOW_STATE_MAXIMIZED;
    }
    if(strcmp(s, "fullscreen") == 0){
        return MC_WM_WINDOW_STATE_FULLSCREEN;
    }

    return (MC_WMWindowState)luaL_error(L, "mc.wm: unknown window state '%s'", s);
}

static void push_size(lua_State *L, MC_Size2U size){
    lua_createtable(L, 0, 2);
    lua_pushinteger(L, size.width);
    lua_setfield(L, -2, "width");
    lua_pushinteger(L, size.height);
    lua_setfield(L, -2, "height");
}

static void push_position(lua_State *L, MC_Vec2i position){
    lua_createtable(L, 0, 2);
    lua_pushinteger(L, position.x);
    lua_setfield(L, -2, "x");
    lua_pushinteger(L, position.y);
    lua_setfield(L, -2, "y");
}

static void push_rect(lua_State *L, MC_Rect2IU rect){
    lua_createtable(L, 0, 4);
    lua_pushinteger(L, rect.x);
    lua_setfield(L, -2, "x");
    lua_pushinteger(L, rect.y);
    lua_setfield(L, -2, "y");
    lua_pushinteger(L, rect.width);
    lua_setfield(L, -2, "width");
    lua_pushinteger(L, rect.height);
    lua_setfield(L, -2, "height");
}

static MC_WindowRef *window_ref(lua_State *L){
    LuaWindow *lw = luaL_checkudata(L, 1, WINDOW_MT);
    if(lw->ref == NULL){
        luaL_error(L, "mc.wm: window is closed");
    }

    return lw->ref;
}

static int win_set_title(lua_State *L){
    MC_WindowRef *ref = window_ref(L);
    size_t len;
    const char *s = luaL_checklstring(L, 2, &len);

    require_ok(L, mc_wm_window_set_title(ref, MC_STR(s, s + len)), "set_title");

    lua_settop(L, 1);
    return 1;
}

static int win_set_size(lua_State *L){
    MC_WindowRef *ref = window_ref(L);
    require_ok(L, mc_wm_window_set_size(ref, check_size(L, 2)), "set_size");

    lua_settop(L, 1);
    return 1;
}

static int win_set_position(lua_State *L){
    MC_WindowRef *ref = window_ref(L);
    require_ok(L, mc_wm_window_set_position(ref, check_position(L, 2)), "set_position");

    lua_settop(L, 1);
    return 1;
}

static int win_set_rect(lua_State *L){
    MC_WindowRef *ref = window_ref(L);
    require_ok(L, mc_wm_window_set_rect(ref, check_rect(L, 2)), "set_rect");

    lua_settop(L, 1);
    return 1;
}

static int win_set_state(lua_State *L){
    MC_WindowRef *ref = window_ref(L);
    require_ok(L, mc_wm_window_set_state(ref, check_state(L, 2)), "set_state");

    lua_settop(L, 1);
    return 1;
}

static int win_get_size(lua_State *L){
    MC_WindowRef *ref = window_ref(L);
    MC_Size2U size;
    require_ok(L, mc_wm_window_get_size(ref, &size), "get_size");

    push_size(L, size);
    return 1;
}

static int win_get_position(lua_State *L){
    MC_WindowRef *ref = window_ref(L);
    MC_Vec2i position;
    require_ok(L, mc_wm_window_get_position(ref, &position), "get_position");

    push_position(L, position);
    return 1;
}

static int win_get_rect(lua_State *L){
    MC_WindowRef *ref = window_ref(L);
    MC_Rect2IU rect;
    require_ok(L, mc_wm_window_get_rect(ref, &rect), "get_rect");

    push_rect(L, rect);
    return 1;
}

static int win_is_alive(lua_State *L){
    LuaWindow *lw = luaL_checkudata(L, 1, WINDOW_MT);
    lua_pushboolean(L, lw->ref != NULL && mc_wm_window_is_alive(lw->ref));
    return 1;
}

static int win_close(lua_State *L){
    MC_WindowRef *ref = window_ref(L);
    require_ok(L, mc_wm_window_close(ref), "close");

    return 0;
}

static int win_destroy(lua_State *L){
    LuaWindow *lw = luaL_checkudata(L, 1, WINDOW_MT);
    if(lw->window){
        mc_wm_window_destroy(lw->window);
        lw->window = NULL;
    }

    return 0;
}

static int win_gc(lua_State *L){
    LuaWindow *lw = luaL_checkudata(L, 1, WINDOW_MT);
    if(lw->ref){
        mc_wm_window_unref(lw->ref);
        lw->ref = NULL;
    }

    return 0;
}

static void push_window(lua_State *L, int wm_index, MC_WindowRef *ref, MC_WMWindow *window){
    LuaWindow *lw = lua_newuserdatauv(L, sizeof(LuaWindow), 1);
    lw->ref = ref;
    lw->window = window;
    luaL_setmetatable(L, WINDOW_MT);

    lua_pushvalue(L, wm_index);
    lua_setiuservalue(L, -2, 1);
}

static int wm_create_window(lua_State *L){
    LuaWM *lwm = luaL_checkudata(L, 1, WM_MT);
    if(lwm->wm == NULL){
        return luaL_error(L, "mc.wm: window manager is destroyed");
    }

    MC_WMWindow *window;
    require_ok(L, mc_wm_window_init(lwm->wm, &window), "create_window");

    MC_WindowRef *ref = mc_wm_window_ref(mc_wm_window_get_ref(window));

    if(!lua_isnoneornil(L, 2)){
        luaL_checktype(L, 2, LUA_TTABLE);

        lua_getfield(L, 2, "title");
        if(!lua_isnil(L, -1)){
            size_t len;
            const char *s = luaL_checklstring(L, -1, &len);
            require_ok(L, mc_wm_window_set_title(ref, MC_STR(s, s + len)), "set_title");
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, "size");
        if(!lua_isnil(L, -1)){
            require_ok(L, mc_wm_window_set_size(ref, check_size(L, lua_gettop(L))), "set_size");
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, "position");
        if(!lua_isnil(L, -1)){
            require_ok(L, mc_wm_window_set_position(ref, check_position(L, lua_gettop(L))), "set_position");
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, "state");
        if(!lua_isnil(L, -1)){
            require_ok(L, mc_wm_window_set_state(ref, check_state(L, lua_gettop(L))), "set_state");
        }
        lua_pop(L, 1);
    }

    push_window(L, 1, ref, window);
    return 1;
}

static int wm_get_focused_window(lua_State *L){
    LuaWM *lwm = luaL_checkudata(L, 1, WM_MT);
    if(lwm->wm == NULL){
        return luaL_error(L, "mc.wm: window manager is destroyed");
    }

    MC_WindowRef *ref = NULL;
    MC_Error e = mc_wm_get_focused_window(lwm->wm, &ref);
    if(e == MCE_NOT_FOUND){
        lua_pushnil(L);
        return 1;
    }
    require_ok(L, e, "get_focused_window");

    push_window(L, 1, ref, NULL);
    return 1;
}

static int wm_poll_event(lua_State *L){
    LuaWM *lwm = luaL_checkudata(L, 1, WM_MT);
    if(lwm->wm == NULL){
        return luaL_error(L, "mc.wm: window manager is destroyed");
    }

    MC_WMEvent event;
    if(mc_wm_poll_event(lwm->wm, &event) != MCE_OK){
        lua_pushnil(L);
        return 1;
    }

    lua_createtable(L, 0, 2);
    lua_pushstring(L, mc_wm_event_type_str(event.type));
    lua_setfield(L, -2, "type");

    if(event.type == MC_WME_KEY_DOWN){
        lua_pushinteger(L, (lua_Integer)event.as.key_down.key);
        lua_setfield(L, -2, "key");
    }
    else if(event.type == MC_WME_KEY_UP){
        lua_pushinteger(L, (lua_Integer)event.as.key_up.key);
        lua_setfield(L, -2, "key");
    }

    return 1;
}

static int wm_destroy(lua_State *L){
    LuaWM *lwm = luaL_checkudata(L, 1, WM_MT);
    if(lwm->wm){
        mc_wm_destroy(lwm->wm);
        lwm->wm = NULL;
    }

    return 0;
}

static int wm_resolve(lua_State *L){
    const char *impl = luaL_optstring(L, 1, NULL);

    MC_WM *wm;
    require_ok(L, impl ? mc_wm_resolve_as(impl, &wm) : mc_wm_resolve(&wm), "resolve");

    LuaWM *lwm = lua_newuserdatauv(L, sizeof(LuaWM), 0);
    lwm->wm = wm;
    luaL_setmetatable(L, WM_MT);
    return 1;
}

static const luaL_Reg wm_methods[] = {
    {"create_window", wm_create_window},
    {"get_focused_window", wm_get_focused_window},
    {"poll_event", wm_poll_event},
    {"destroy", wm_destroy},
    {"__gc", wm_destroy},
    {NULL, NULL},
};

static const luaL_Reg window_methods[] = {
    {"set_title", win_set_title},
    {"set_size", win_set_size},
    {"set_position", win_set_position},
    {"set_rect", win_set_rect},
    {"set_state", win_set_state},
    {"get_size", win_get_size},
    {"get_position", win_get_position},
    {"get_rect", win_get_rect},
    {"is_alive", win_is_alive},
    {"close", win_close},
    {"destroy", win_destroy},
    {"__gc", win_gc},
    {NULL, NULL},
};

static void register_class(lua_State *L, const char *name, const luaL_Reg *methods){
    luaL_newmetatable(L, name);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, methods, 0);
    lua_pop(L, 1);
}

int mc_wm_lua_module(lua_State *L, const MC_WMVtab *vtab){
    static const luaL_Reg module[] = {
        {"resolve", wm_resolve},
        {NULL, NULL},
    };

    if(vtab != NULL){
        mc_wm_resolver_register(vtab);
    }

    register_class(L, WM_MT, wm_methods);
    register_class(L, WINDOW_MT, window_methods);

    luaL_newlib(L, module);
    return 1;
}

void mc_wm_lua_open(lua_State *L, const MC_WMVtab *vtab){
    luaL_getsubtable(L, LUA_REGISTRYINDEX, LUA_LOADED_TABLE);
    mc_wm_lua_module(L, vtab);
    lua_setfield(L, -2, "mc.wm");
    lua_pop(L, 1);
}
