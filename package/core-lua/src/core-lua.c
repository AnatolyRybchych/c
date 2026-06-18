#include <mc/core-lua/core-lua.h>

#include <mc/sched.h>
#include <mc/time.h>
#include <mc/data/alloc.h>
#include <mc/data/str.h>
#include <mc/data/json.h>
#include <mc/data/stream.h>
#include <mc/data/mstream.h>

#include <lua.h>
#include <lauxlib.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SCHED_MT "mc.core.sched"
#define TASK_MT  "mc.core.task"

#define TASK_DEPS_MAX 32

typedef struct LuaSched {
    MC_Sched *sched;
    bool owned;
} LuaSched;

typedef struct LuaTask {
    MC_Task *task;
} LuaTask;

typedef struct LuaTaskCtx {
    lua_State *L;
    int fn_ref;
    int task_ref;
} LuaTaskCtx;

static const char *status_str(MC_TaskStatus status) {
    switch (status) {
    case MC_TASK_CONTINUE:
        return "continue";
    case MC_TASK_SUSPEND:
        return "suspend";
    default:
        return "done";
    }
}

static MC_Time ms_to_time(lua_Integer ms) {
    if (ms < 0) {
        ms = 0;
    }

    return (MC_Time){
        .sec = (uint64_t)(ms / MC_MSEC_IN_SEC),
        .nsec = (uint64_t)(ms % MC_MSEC_IN_SEC) * (MC_NSEC_IN_SEC / MC_MSEC_IN_SEC),
    };
}

static MC_Task *check_task(lua_State *L, int idx) {
    LuaTask *t = luaL_checkudata(L, idx, TASK_MT);
    if (t->task == NULL) {
        luaL_error(L, "mc.core: task is gone");
    }

    return t->task;
}

static MC_TaskStatus lua_task_do_some(MC_Task *task) {
    LuaTaskCtx *ctx = mc_task_data(task, NULL);
    if (ctx->fn_ref == LUA_NOREF) {
        return MC_TASK_DONE;
    }

    lua_State *L = ctx->L;
    lua_rawgeti(L, LUA_REGISTRYINDEX, ctx->fn_ref);
    lua_rawgeti(L, LUA_REGISTRYINDEX, ctx->task_ref);

    MC_TaskStatus status = MC_TASK_DONE;
    if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
        fprintf(stderr, "mc.core: task error: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    } else {
        const char *s = lua_isstring(L, -1) ? lua_tostring(L, -1) : NULL;
        if (s != NULL && strcmp(s, "continue") == 0) {
            status = MC_TASK_CONTINUE;
        } else if (s != NULL && strcmp(s, "suspend") == 0) {
            status = MC_TASK_SUSPEND;
        }
        lua_pop(L, 1);
    }

    if (status == MC_TASK_DONE) {
        luaL_unref(L, LUA_REGISTRYINDEX, ctx->fn_ref);
        luaL_unref(L, LUA_REGISTRYINDEX, ctx->task_ref);
        ctx->fn_ref = LUA_NOREF;
        ctx->task_ref = LUA_NOREF;
    }

    return status;
}

static int l_sched_task(lua_State *L) {
    LuaSched *s = luaL_checkudata(L, 1, SCHED_MT);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    if (s->sched == NULL) {
        return luaL_error(L, "mc.core: scheduler is destroyed");
    }

    lua_settop(L, 3);

    lua_pushvalue(L, 2);
    int fn_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    LuaTaskCtx ctx = { .L = L, .fn_ref = fn_ref, .task_ref = LUA_NOREF };
    MC_Task *task = NULL;
    if (mc_task_new(s->sched, &task, lua_task_do_some, sizeof(ctx), &ctx) != MCE_OK) {
        luaL_unref(L, LUA_REGISTRYINDEX, fn_ref);
        return luaL_error(L, "mc.core: failed to create task");
    }

    LuaTask *t = lua_newuserdatauv(L, sizeof(*t), 2);
    t->task = task;
    lua_pushvalue(L, 1);
    lua_setiuservalue(L, -2, 1);
    lua_pushvalue(L, 3);
    lua_setiuservalue(L, -2, 2);
    luaL_setmetatable(L, TASK_MT);

    lua_pushvalue(L, -1);
    ((LuaTaskCtx *)mc_task_data(task, NULL))->task_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    return 1;
}

static int l_sched_run(lua_State *L) {
    LuaSched *s = luaL_checkudata(L, 1, SCHED_MT);
    if (s->sched == NULL) {
        return luaL_error(L, "mc.core: scheduler is destroyed");
    }

    mc_sched_run(s->sched);
    return 0;
}

static int l_sched_step(lua_State *L) {
    LuaSched *s = luaL_checkudata(L, 1, SCHED_MT);
    if (s->sched == NULL) {
        return luaL_error(L, "mc.core: scheduler is destroyed");
    }

    lua_pushstring(L, status_str(mc_sched_continue(s->sched)));
    return 1;
}

static int l_sched_wait(lua_State *L) {
    LuaSched *s = luaL_checkudata(L, 1, SCHED_MT);
    if (s->sched == NULL) {
        return luaL_error(L, "mc.core: scheduler is destroyed");
    }

    MC_Time timeout;
    const MC_Time *timeout_ptr = NULL;
    if (!lua_isnoneornil(L, 2)) {
        timeout = ms_to_time(luaL_checkinteger(L, 2));
        timeout_ptr = &timeout;
    }

    int count = lua_gettop(L) - 2;
    if (count <= 0) {
        return luaL_error(L, "mc.core: sched:wait needs at least one task");
    }
    if (count > TASK_DEPS_MAX) {
        return luaL_error(L, "mc.core: sched:wait accepts at most %d tasks", TASK_DEPS_MAX);
    }

    MC_Task *tasks[TASK_DEPS_MAX];
    for (int i = 0; i < count; i++) {
        tasks[i] = check_task(L, i + 3);
    }

    MC_Error err = mc_task_waitn(timeout_ptr, (size_t)count, tasks);
    if (err != MCE_OK && err != MCE_TIMEOUT) {
        return luaL_error(L, "mc.core: sched:wait failed");
    }

    lua_pushboolean(L, err == MCE_OK);
    return 1;
}

static int l_sched_gc(lua_State *L) {
    LuaSched *s = luaL_checkudata(L, 1, SCHED_MT);
    if (s->sched != NULL && s->owned) {
        mc_sched_delete(s->sched);
    }

    s->sched = NULL;
    return 0;
}

static int l_task_run(lua_State *L) {
    MC_Task *task = check_task(L, 1);
    if (mc_task_run(task) != MCE_OK) {
        return luaL_error(L, "mc.core: task:run failed");
    }

    lua_settop(L, 1);
    return 1;
}

static int l_task_schedule(lua_State *L) {
    MC_Task *task = check_task(L, 1);
    lua_Integer timeout = luaL_checkinteger(L, 2);
    if (mc_task_sched(task, ms_to_time(timeout)) != MCE_OK) {
        return luaL_error(L, "mc.core: task:schedule failed");
    }

    lua_settop(L, 1);
    return 1;
}

static int l_task_run_after(lua_State *L) {
    MC_Task *task = check_task(L, 1);
    int count = lua_gettop(L) - 1;
    if (count <= 0) {
        return luaL_error(L, "mc.core: task:run_after needs at least one task");
    }
    if (count > TASK_DEPS_MAX) {
        return luaL_error(L, "mc.core: task:run_after accepts at most %d tasks", TASK_DEPS_MAX);
    }

    MC_Task *deps[TASK_DEPS_MAX];
    for (int i = 0; i < count; i++) {
        deps[i] = check_task(L, i + 2);
    }

    if (mc_task_run_aftern(task, (size_t)count, deps) != MCE_OK) {
        return luaL_error(L, "mc.core: task:run_after failed");
    }

    lua_settop(L, 1);
    return 1;
}

static int l_task_delay(lua_State *L) {
    MC_Task *task = check_task(L, 1);
    lua_Integer delay = luaL_checkinteger(L, 2);
    if (mc_task_delay(task, ms_to_time(delay)) != MCE_OK) {
        return luaL_error(L, "mc.core: task:delay failed");
    }

    lua_settop(L, 1);
    return 1;
}

static int l_task_status(lua_State *L) {
    MC_Task *task = check_task(L, 1);
    lua_pushstring(L, status_str(mc_task_status(task)));
    return 1;
}

static int l_task_index(lua_State *L) {
    const char *key = lua_tostring(L, 2);
    if (key != NULL && strcmp(key, "sched") == 0) {
        lua_getiuservalue(L, 1, 1);
        return 1;
    }
    if (key != NULL && strcmp(key, "data") == 0) {
        lua_getiuservalue(L, 1, 2);
        return 1;
    }

    lua_pushvalue(L, 2);
    lua_gettable(L, lua_upvalueindex(1));
    return 1;
}

static int l_task_newindex(lua_State *L) {
    const char *key = lua_tostring(L, 2);
    if (key != NULL && strcmp(key, "data") == 0) {
        lua_pushvalue(L, 3);
        lua_setiuservalue(L, 1, 2);
        return 0;
    }

    return luaL_error(L, "mc.core: task has no writable field '%s'", key != NULL ? key : "?");
}

static int l_task_gc(lua_State *L) {
    LuaTask *t = luaL_checkudata(L, 1, TASK_MT);
    if (t->task != NULL) {
        mc_task_unref(t->task);
        t->task = NULL;
    }

    return 0;
}

static int l_sched_new(lua_State *L) {
    LuaSched *s = lua_newuserdatauv(L, sizeof(*s), 0);
    s->sched = NULL;
    s->owned = true;
    luaL_setmetatable(L, SCHED_MT);

    if (mc_sched_new(&s->sched) != MCE_OK) {
        return luaL_error(L, "mc.core: failed to create scheduler");
    }

    return 1;
}

static void register_sched_class(lua_State *L) {
    static const luaL_Reg methods[] = {
        { "task", l_sched_task },
        { "run", l_sched_run },
        { "step", l_sched_step },
        { "wait", l_sched_wait },
        { "__gc", l_sched_gc },
        { NULL, NULL },
    };

    luaL_newmetatable(L, SCHED_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, methods, 0);
    lua_pop(L, 1);
}

static void register_task_class(lua_State *L) {
    static const luaL_Reg methods[] = {
        { "run", l_task_run },
        { "schedule", l_task_schedule },
        { "run_after", l_task_run_after },
        { "delay", l_task_delay },
        { "status", l_task_status },
        { NULL, NULL },
    };

    luaL_newmetatable(L, TASK_MT);

    lua_pushcfunction(L, l_task_gc);
    lua_setfield(L, -2, "__gc");

    luaL_newlib(L, methods);
    lua_pushcclosure(L, l_task_index, 1);
    lua_setfield(L, -2, "__index");

    lua_pushcfunction(L, l_task_newindex);
    lua_setfield(L, -2, "__newindex");

    lua_pop(L, 1);
}

static void push_json(lua_State *L, MC_Json *json) {
    luaL_checkstack(L, 4, "mc.core.json: nesting too deep");

    switch (mc_json_type(json)) {
    case MC_JSON_BOOL: {
        bool value = false;
        mc_json_bool(json, &value);
        lua_pushboolean(L, value);
        break;
    }
    case MC_JSON_NUMBER: {
        if (mc_json_is_integer(json)) {
            int64_t value = 0;
            if (mc_json_i64(json, &value) == MCE_OK) {
                lua_pushinteger(L, (lua_Integer)value);
                break;
            }
        }

        double number = 0;
        mc_json_number(json, &number);
        lua_pushnumber(L, (lua_Number)number);
        break;
    }
    case MC_JSON_STRING: {
        MC_Str str = {0};
        mc_json_str(json, &str);
        lua_pushlstring(L, str.beg, MC_STR_LEN(str));
        break;
    }
    case MC_JSON_LIST: {
        size_t length = mc_json_length(json);
        lua_createtable(L, (int)length, 0);
        for (size_t i = 0; i < length; i++) {
            MC_Json *item = NULL;
            mc_json_at(json, i, &item);
            push_json(L, item);
            lua_rawseti(L, -2, (lua_Integer)(i + 1));
        }
        break;
    }
    case MC_JSON_OBJECT: {
        size_t length = mc_json_length(json);
        lua_createtable(L, 0, (int)length);
        for (size_t i = 0; i < length; i++) {
            MC_Str key = {0};
            MC_Json *value = NULL;
            mc_json_object_at(json, i, &key, &value);
            lua_pushlstring(L, key.beg, MC_STR_LEN(key));
            push_json(L, value);
            lua_rawset(L, -3);
        }
        break;
    }
    default:
        lua_pushnil(L);
        break;
    }
}

static bool table_is_array(lua_State *L, int idx) {
    size_t length = lua_rawlen(L, idx);
    size_t count = 0;

    lua_pushnil(L);
    while (lua_next(L, idx) != 0) {
        bool indexed = lua_isinteger(L, -2);
        if (indexed) {
            lua_Integer key = lua_tointeger(L, -2);
            indexed = key >= 1 && (size_t)key <= length;
        }

        if (!indexed) {
            lua_pop(L, 2);
            return false;
        }

        count++;
        lua_pop(L, 1);
    }

    return count == length;
}

static MC_Error lua_to_json(lua_State *L, int idx, MC_Json *out) {
    idx = lua_absindex(L, idx);

    switch (lua_type(L, idx)) {
    case LUA_TNIL:
        return mc_json_set_null(out);
    case LUA_TBOOLEAN:
        return mc_json_set_bool(out, lua_toboolean(L, idx));
    case LUA_TNUMBER:
        if (lua_isinteger(L, idx)) {
            return mc_json_set_i64(out, (int64_t)lua_tointeger(L, idx));
        }

        return mc_json_set_lf(out, (double)lua_tonumber(L, idx));
    case LUA_TSTRING: {
        size_t len = 0;
        const char *s = lua_tolstring(L, idx, &len);
        return mc_json_set_string(out, MC_STR(s, s + len));
    }
    case LUA_TTABLE:
        break;
    default:
        return MCE_INVALID_INPUT;
    }

    luaL_checkstack(L, 4, "mc.core.json: nesting too deep");

    if (table_is_array(L, idx)) {
        MC_Error err = mc_json_set_list(out);
        if (err != MCE_OK) {
            return err;
        }

        size_t length = lua_rawlen(L, idx);
        for (size_t i = 1; i <= length; i++) {
            MC_Json *item = NULL;
            err = mc_json_list_add_new(out, &item);
            if (err != MCE_OK) {
                return err;
            }

            lua_rawgeti(L, idx, (lua_Integer)i);
            err = lua_to_json(L, -1, item);
            lua_pop(L, 1);
            if (err != MCE_OK) {
                return err;
            }
        }

        return MCE_OK;
    }

    MC_Error err = mc_json_set_object(out);
    if (err != MCE_OK) {
        return err;
    }

    lua_pushnil(L);
    while (lua_next(L, idx) != 0) {
        if (lua_type(L, -2) != LUA_TSTRING) {
            lua_pop(L, 1);
            continue;
        }

        MC_Json *item = NULL;
        err = mc_json_object_add_new(out, &item, "%s", lua_tolstring(L, -2, NULL));
        if (err != MCE_OK) {
            lua_pop(L, 2);
            return err;
        }

        err = lua_to_json(L, -1, item);
        if (err != MCE_OK) {
            lua_pop(L, 2);
            return err;
        }

        lua_pop(L, 1);
    }

    return MCE_OK;
}

static int l_json_loads(lua_State *L) {
    size_t len = 0;
    const char *s = luaL_checklstring(L, 1, &len);

    MC_Json *json = NULL;
    if (mc_json_loads(&mc_alloc_malloc, &json, MC_STR(s, s + len)) != MCE_OK) {
        return luaL_error(L, "mc.core.json: invalid json");
    }

    push_json(L, json);
    mc_json_delete(&json);
    return 1;
}

static int l_json_dumps(lua_State *L) {
    luaL_checkany(L, 1);

    MC_Json *json = NULL;
    if (mc_json_new(&mc_alloc_malloc, &json) != MCE_OK) {
        return luaL_error(L, "mc.core.json: out of memory");
    }

    if (lua_to_json(L, 1, json) != MCE_OK) {
        mc_json_delete(&json);
        return luaL_error(L, "mc.core.json: value is not serializable");
    }

    MC_Stream *out = NULL;
    if (mc_mstream(&mc_alloc_malloc, &out) != MCE_OK) {
        mc_json_delete(&json);
        return luaL_error(L, "mc.core.json: out of memory");
    }

    MC_Error err = mc_json_dump(json, out);
    mc_json_delete(&json);
    if (err != MCE_OK) {
        mc_close(out);
        return luaL_error(L, "mc.core.json: dump failed");
    }

    size_t size = mc_mstream_size(out);
    mc_set_cursor(out, 0, MC_CURSOR_FROM_BEG);

    luaL_Buffer b;
    char *dst = luaL_buffinitsize(L, &b, size);
    size_t read = 0;
    mc_read(out, size, dst, &read);
    mc_close(out);

    luaL_pushresultsize(&b, read);
    return 1;
}

int mc_core_lua_module(lua_State *L) {
    static const luaL_Reg module[] = {
        { "sched", l_sched_new },
        { NULL, NULL },
    };

    static const luaL_Reg json_funcs[] = {
        { "loads", l_json_loads },
        { "dumps", l_json_dumps },
        { NULL, NULL },
    };

    register_sched_class(L);
    register_task_class(L);

    luaL_newlib(L, module);

    luaL_newlib(L, json_funcs);
    lua_setfield(L, -2, "json");

    return 1;
}

int mc_core_lua_push_sched(lua_State *L, MC_Sched *sched) {
    LuaSched *s = lua_newuserdatauv(L, sizeof(*s), 0);
    s->sched = sched;
    s->owned = false;
    luaL_setmetatable(L, SCHED_MT);

    lua_pushvalue(L, -1);
    return luaL_ref(L, LUA_REGISTRYINDEX);
}

MC_Sched *mc_core_lua_pop_sched(lua_State *L, int ref) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    LuaSched *s = luaL_testudata(L, -1, SCHED_MT);
    lua_pop(L, 1);

    luaL_unref(L, LUA_REGISTRYINDEX, ref);

    if (s == NULL) {
        return NULL;
    }

    MC_Sched *sched = s->sched;
    s->sched = NULL;
    return sched;
}

void mc_core_lua_open(lua_State *L) {
    luaL_getsubtable(L, LUA_REGISTRYINDEX, LUA_LOADED_TABLE);
    mc_core_lua_module(L);
    lua_setfield(L, -2, "mc.core");
    lua_pop(L, 1);
}
