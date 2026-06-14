#include <mc/wm-lua/wm-lua.h>

#include <lua.h>
#include <lauxlib.h>

#include <mc/wm/wm.h>
#include <mc/wm/event.h>
#include <mc/wm/resolver.h>
#include <mc/data/str.h>
#include <mc/data/json.h>

#include <string.h>
#include <stdio.h>

#define WM_MT           "mc.wm.WM"
#define WINDOW_MT       "mc.wm.Window"
#define SUBSCRIPTION_MT "mc.wm.Subscription"

typedef struct LuaWM{
    MC_WMRef *wm;
} LuaWM;

typedef struct LuaWindow{
    MC_WindowRef *ref;
    MC_WMWindow *window;
} LuaWindow;

#define OBJECT_CACHE "mc.wm.objects"

static void push_object_cache(lua_State *L){
    if(luaL_getsubtable(L, LUA_REGISTRYINDEX, OBJECT_CACHE)){
        return;
    }

    lua_newtable(L);
    lua_pushstring(L, "v");
    lua_setfield(L, -2, "__mode");
    lua_setmetatable(L, -2);
}

static bool cache_get(lua_State *L, const void *key){
    push_object_cache(L);
    if(lua_rawgetp(L, -1, key) == LUA_TNIL){
        lua_pop(L, 2);
        return false;
    }

    lua_remove(L, -2);
    return true;
}

static void cache_put(lua_State *L, const void *key){
    push_object_cache(L);
    lua_pushvalue(L, -2);
    lua_rawsetp(L, -2, key);
    lua_pop(L, 1);
}

static void cache_remove(lua_State *L, const void *key){
    push_object_cache(L);
    lua_pushnil(L);
    lua_rawsetp(L, -2, key);
    lua_pop(L, 1);
}

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

static const char *state_str(MC_WMWindowState state){
    switch(state){
    case MC_WM_WINDOW_STATE_NORMAL: return "normal";
    case MC_WM_WINDOW_STATE_MINIMIZED: return "minimized";
    case MC_WM_WINDOW_STATE_MAXIMIZED: return "maximized";
    case MC_WM_WINDOW_STATE_FULLSCREEN: return "fullscreen";
    default: return "unknown";
    }
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

MC_WindowRef *mc_wm_lua_check_window(lua_State *L, int idx){
    LuaWindow *lw = luaL_checkudata(L, idx, WINDOW_MT);
    if(lw->ref == NULL){
        luaL_error(L, "mc.wm: window is closed");
    }

    return lw->ref;
}

static MC_WindowRef *window_ref(lua_State *L){
    return mc_wm_lua_check_window(L, 1);
}

static int win_set_title(lua_State *L){
    MC_WindowRef *ref = window_ref(L);
    size_t len;
    const char *s = luaL_checklstring(L, 2, &len);

    require_ok(L, mc_wm_window_set_title(ref, MC_STR(s, s + len)), "set_title");

    lua_settop(L, 1);
    return 1;
}

static MC_WMArea check_area(lua_State *L, int idx){
    const char *s = luaL_optstring(L, idx, "window");
    if(strcmp(s, "window") == 0){
        return MC_WM_AREA_WINDOW;
    }
    if(strcmp(s, "decorated") == 0){
        return MC_WM_AREA_DECORATED;
    }
    if(strcmp(s, "drawable") == 0){
        return MC_WM_AREA_DRAWABLE;
    }

    return (MC_WMArea)luaL_error(L, "mc.wm: unknown window area '%s' (want window/decorated/drawable)", s);
}

static int win_set_rect(lua_State *L){
    MC_WindowRef *ref = window_ref(L);
    MC_WMArea area = check_area(L, 3);
    require_ok(L, mc_wm_window_set_rect(ref, area, check_rect(L, 2)), "set_rect");

    lua_settop(L, 1);
    return 1;
}

static int win_set_size(lua_State *L){
    MC_WindowRef *ref = window_ref(L);
    MC_WMArea area = check_area(L, 3);
    require_ok(L, mc_wm_window_set_size(ref, area, check_size(L, 2)), "set_size");

    lua_settop(L, 1);
    return 1;
}

static int win_set_position(lua_State *L){
    MC_WindowRef *ref = window_ref(L);
    MC_WMArea area = check_area(L, 3);
    require_ok(L, mc_wm_window_set_position(ref, area, check_position(L, 2)), "set_position");

    lua_settop(L, 1);
    return 1;
}

static int win_set_state(lua_State *L){
    MC_WindowRef *ref = window_ref(L);
    require_ok(L, mc_wm_window_set_state(ref, check_state(L, 2)), "set_state");

    lua_settop(L, 1);
    return 1;
}

static int win_get_rect(lua_State *L){
    MC_WindowRef *ref = window_ref(L);
    MC_Rect2IU rect;
    require_ok(L, mc_wm_window_get_rect(ref, check_area(L, 2), &rect), "get_rect");

    push_rect(L, rect);
    return 1;
}

static int win_get_size(lua_State *L){
    MC_WindowRef *ref = window_ref(L);
    MC_Size2U size;
    require_ok(L, mc_wm_window_get_size(ref, check_area(L, 2), &size), "get_size");

    push_size(L, size);
    return 1;
}

static int win_get_position(lua_State *L){
    MC_WindowRef *ref = window_ref(L);
    MC_Vec2i position;
    require_ok(L, mc_wm_window_get_position(ref, check_area(L, 2), &position), "get_position");

    push_position(L, position);
    return 1;
}

static int win_get_title(lua_State *L){
    MC_WindowRef *ref = window_ref(L);
    char buf[1024];
    size_t len = 0;
    require_ok(L, mc_wm_window_get_title(ref, buf, sizeof(buf), &len), "get_title");

    lua_pushlstring(L, buf, len);
    return 1;
}

static int win_get_state(lua_State *L){
    MC_WindowRef *ref = window_ref(L);
    MC_WMWindowState state;
    require_ok(L, mc_wm_window_get_state(ref, &state), "get_state");

    lua_pushstring(L, state_str(state));
    return 1;
}

static int win_is_alive(lua_State *L){
    LuaWindow *lw = luaL_checkudata(L, 1, WINDOW_MT);
    lua_pushboolean(L, lw->ref != NULL && mc_wm_window_is_alive(lw->ref));
    return 1;
}

static int win_is_system(lua_State *L){
    MC_WindowRef *ref = window_ref(L);
    bool sys = false;
    require_ok(L, mc_wm_window_is_system(ref, &sys), "is_system");

    lua_pushboolean(L, sys);
    return 1;
}

static int win_close(lua_State *L){
    MC_WindowRef *ref = window_ref(L);
    require_ok(L, mc_wm_window_close(ref), "close");

    return 0;
}

static int win_focus(lua_State *L){
    MC_WindowRef *ref = window_ref(L);
    require_ok(L, mc_wm_window_focus(ref), "focus");

    lua_settop(L, 1);
    return 1;
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
        cache_remove(L, lw->ref);
        mc_wm_window_unref(lw->ref);
        lw->ref = NULL;
    }

    return 0;
}

static void push_window(lua_State *L, int wm_index, MC_WindowRef *ref, MC_WMWindow *window){
    if(cache_get(L, ref)){
        LuaWindow *cached = luaL_checkudata(L, -1, WINDOW_MT);
        if(window != NULL && cached->window == NULL){
            cached->window = window;
        }

        mc_wm_window_unref(ref);
        return;
    }

    LuaWindow *lw = lua_newuserdatauv(L, sizeof(LuaWindow), 1);
    lw->ref = ref;
    lw->window = window;
    luaL_setmetatable(L, WINDOW_MT);

    if(wm_index != 0){
        lua_pushvalue(L, wm_index);
        lua_setiuservalue(L, -2, 1);
    }

    cache_put(L, ref);
}

void mc_wm_lua_push_window(lua_State *L, MC_WindowRef *window){
    push_window(L, 0, window, NULL);
}

static void push_json(lua_State *L, MC_Json *json){
    switch(mc_json_type(json)){
    case MC_JSON_BOOL:{
        bool value = false;
        mc_json_bool(json, &value);
        lua_pushboolean(L, value);
        return;
    }
    case MC_JSON_NUMBER:
        if(mc_json_is_integer(json)){
            int64_t value = 0;
            mc_json_i64(json, &value);
            lua_pushinteger(L, (lua_Integer)value);
        }
        else{
            double value = 0;
            mc_json_f64(json, &value);
            lua_pushnumber(L, value);
        }
        return;
    case MC_JSON_STRING:{
        MC_Str s = MC_STRC("");
        mc_json_str(json, &s);
        lua_pushlstring(L, s.beg, (size_t)(s.end - s.beg));
        return;
    }
    case MC_JSON_LIST:{
        size_t n = mc_json_length(json);
        lua_createtable(L, (int)n, 0);
        for(size_t i = 0; i < n; i++){
            MC_Json *item = NULL;
            mc_json_at(json, i, &item);
            push_json(L, item);
            lua_rawseti(L, -2, (lua_Integer)(i + 1));
        }
        return;
    }
    case MC_JSON_OBJECT:{
        size_t n = mc_json_length(json);
        lua_createtable(L, 0, (int)n);
        for(size_t i = 0; i < n; i++){
            MC_Str key = MC_STRC("");
            MC_Json *value = NULL;
            mc_json_object_at(json, i, &key, &value);
            lua_pushlstring(L, key.beg, (size_t)(key.end - key.beg));
            push_json(L, value);
            lua_settable(L, -3);
        }
        return;
    }
    default:
        lua_pushnil(L);
        return;
    }
}

void mc_wm_lua_push_json(lua_State *L, MC_Json *json){
    push_json(L, json);
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
            require_ok(L, mc_wm_window_set_size(ref, MC_WM_AREA_WINDOW, check_size(L, lua_gettop(L))), "set_size");
        }
        lua_pop(L, 1);

        lua_getfield(L, 2, "position");
        if(!lua_isnil(L, -1)){
            require_ok(L, mc_wm_window_set_position(ref, MC_WM_AREA_WINDOW, check_position(L, lua_gettop(L))), "set_position");
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

static int wm_get_hovered_window(lua_State *L){
    LuaWM *lwm = luaL_checkudata(L, 1, WM_MT);
    if(lwm->wm == NULL){
        return luaL_error(L, "mc.wm: window manager is destroyed");
    }

    MC_WindowRef *ref = NULL;
    MC_Error e = mc_wm_get_hovered_window(lwm->wm, &ref);
    if(e == MCE_NOT_FOUND){
        lua_pushnil(L);
        return 1;
    }
    require_ok(L, e, "get_hovered_window");

    push_window(L, 1, ref, NULL);
    return 1;
}

typedef struct CollectWindows{
    lua_State *L;
    int table_index;
    lua_Integer count;
} CollectWindows;

static MC_Error collect_window(MC_WindowRef *window, void *ctx){
    CollectWindows *collect = ctx;

    push_window(collect->L, 1, window, NULL);
    lua_rawseti(collect->L, collect->table_index, ++collect->count);
    return MCE_OK;
}

static int wm_get_all_windows(lua_State *L){
    LuaWM *lwm = luaL_checkudata(L, 1, WM_MT);
    if(lwm->wm == NULL){
        return luaL_error(L, "mc.wm: window manager is destroyed");
    }

    lua_newtable(L);
    CollectWindows collect = {.L = L, .table_index = lua_gettop(L), .count = 0};
    require_ok(L, mc_wm_get_all_windows(lwm->wm, collect_window, &collect), "get_all_windows");

    return 1;
}

typedef struct LuaSubscription{
    MC_WMRef *wm;
    MC_WMEventSubscription *sub;
    lua_State *L;
    int ref;
} LuaSubscription;

static int event_window_index(lua_State *L){
    const char *key = lua_tostring(L, 2);
    if(key == NULL || strcmp(key, "window") != 0){
        lua_pushnil(L);
        return 1;
    }

    MC_WMRef *wm = lua_touserdata(L, lua_upvalueindex(1));
    uint64_t identity = (uint64_t)lua_tointeger(L, lua_upvalueindex(2));

    MC_WindowRef *ref = NULL;
    if(wm == NULL || mc_wm_resolve_window(wm, identity, &ref) != MCE_OK){
        lua_pushnil(L);
    }
    else{
        mc_wm_lua_push_window(L, ref);
    }

    lua_pushvalue(L, 2);
    lua_pushvalue(L, -2);
    lua_rawset(L, 1);

    lua_pushnil(L);
    lua_setmetatable(L, 1);

    return 1;
}

static void push_event(lua_State *L, MC_WMRef *wm, const MC_WMEvent *event){
    MC_Json *json = NULL;
    if(mc_wm_event_to_json(wm, NULL, event, &json) != MCE_OK){
        lua_newtable(L);
        return;
    }

    push_json(L, json);
    mc_json_delete(&json);

    lua_getfield(L, -1, "window");
    if(!lua_isinteger(L, -1)){
        lua_pop(L, 1);
        return;
    }

    uint64_t identity = (uint64_t)lua_tointeger(L, -1);
    lua_pop(L, 1);

    lua_pushnil(L);
    lua_setfield(L, -2, "window");

    lua_createtable(L, 0, 1);
    lua_pushlightuserdata(L, wm);
    lua_pushinteger(L, (lua_Integer)identity);
    lua_pushcclosure(L, event_window_index, 2);
    lua_setfield(L, -2, "__index");
    lua_setmetatable(L, -2);
}

static void lua_event_cb(MC_WMRef *wm, const MC_WMEvent *event, void *user_data){
    LuaSubscription *s = user_data;
    if(s->ref == LUA_NOREF){
        return;
    }

    lua_rawgeti(s->L, LUA_REGISTRYINDEX, s->ref);
    push_event(s->L, wm, event);
    if(lua_pcall(s->L, 1, 0, 0) != LUA_OK){
        fprintf(stderr, "mc.wm: event callback error: %s\n", lua_tostring(s->L, -1));
        lua_pop(s->L, 1);
    }
}

static void subscription_release(LuaSubscription *s){
    if(s->sub != NULL){
        mc_wm_unsubscribe_event(s->sub);
        s->sub = NULL;
    }

    if(s->ref != LUA_NOREF){
        luaL_unref(s->L, LUA_REGISTRYINDEX, s->ref);
        s->ref = LUA_NOREF;
    }
}

static int sub_unsubscribe(lua_State *L){
    subscription_release(luaL_checkudata(L, 1, SUBSCRIPTION_MT));
    return 0;
}

static int sub_gc(lua_State *L){
    subscription_release(luaL_checkudata(L, 1, SUBSCRIPTION_MT));
    return 0;
}

static int wm_on_event(lua_State *L){
    LuaWM *lwm = luaL_checkudata(L, 1, WM_MT);
    if(lwm->wm == NULL){
        return luaL_error(L, "mc.wm: window manager is destroyed");
    }

    MC_WMEventMatch match = { .type = MC_WME_NONE };
    if(!lua_isnoneornil(L, 2)){
        const char *name = luaL_checkstring(L, 2);
        match.type = mc_wm_event_type_from_str(lwm->wm, name);
        if(match.type == MC_WME_NONE){
            return luaL_error(L, "mc.wm: unknown event type '%s'", name);
        }
    }

    luaL_checktype(L, 3, LUA_TFUNCTION);

    LuaSubscription *s = lua_newuserdatauv(L, sizeof(LuaSubscription), 1);
    s->wm = lwm->wm;
    s->sub = NULL;
    s->L = L;
    s->ref = LUA_NOREF;
    luaL_setmetatable(L, SUBSCRIPTION_MT);

    lua_pushvalue(L, 1);
    lua_setiuservalue(L, -2, 1);

    lua_pushvalue(L, 3);
    s->ref = luaL_ref(L, LUA_REGISTRYINDEX);

    if(mc_wm_subscribe_event(lwm->wm, match, lua_event_cb, s, &s->sub) != MCE_OK){
        luaL_unref(L, LUA_REGISTRYINDEX, s->ref);
        s->ref = LUA_NOREF;
        return luaL_error(L, "mc.wm: on_event failed");
    }

    return 1;
}

static int wm_destroy(lua_State *L){
    LuaWM *lwm = luaL_checkudata(L, 1, WM_MT);
    if(lwm->wm){
        cache_remove(L, lwm->wm);
        mc_wm_unref(lwm->wm);
        lwm->wm = NULL;
    }

    return 0;
}

static int wm_resolve(lua_State *L){
    const char *impl = luaL_optstring(L, 1, NULL);

    MC_WMRef *wm;
    require_ok(L, impl ? mc_wm_resolve_as(impl, &wm) : mc_wm_resolve(&wm), "resolve");

    if(cache_get(L, wm)){
        mc_wm_unref(wm);
        return 1;
    }

    LuaWM *lwm = lua_newuserdatauv(L, sizeof(LuaWM), 0);
    lwm->wm = wm;
    luaL_setmetatable(L, WM_MT);

    cache_put(L, wm);
    return 1;
}

static const luaL_Reg wm_methods[] = {
    {"create_window", wm_create_window},
    {"get_focused_window", wm_get_focused_window},
    {"get_hovered_window", wm_get_hovered_window},
    {"get_all_windows", wm_get_all_windows},
    {"on_event", wm_on_event},
    {"destroy", wm_destroy},
    {"__gc", wm_destroy},
    {NULL, NULL},
};

static const luaL_Reg subscription_methods[] = {
    {"unsubscribe", sub_unsubscribe},
    {"__gc", sub_gc},
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
    {"get_title", win_get_title},
    {"get_state", win_get_state},
    {"is_alive", win_is_alive},
    {"is_system", win_is_system},
    {"close", win_close},
    {"focus", win_focus},
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
    register_class(L, SUBSCRIPTION_MT, subscription_methods);

    luaL_newlib(L, module);
    return 1;
}

void mc_wm_lua_open(lua_State *L, const MC_WMVtab *vtab){
    luaL_getsubtable(L, LUA_REGISTRYINDEX, LUA_LOADED_TABLE);
    mc_wm_lua_module(L, vtab);
    lua_setfield(L, -2, "mc.wm");
    lua_pop(L, 1);
}
