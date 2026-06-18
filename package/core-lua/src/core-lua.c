#include <mc/core-lua/core-lua.h>

#include <mc/sched.h>
#include <mc/time.h>

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

int mc_core_lua_module(lua_State *L) {
    static const luaL_Reg module[] = {
        { "sched", l_sched_new },
        { NULL, NULL },
    };

    register_sched_class(L);
    register_task_class(L);

    luaL_newlib(L, module);
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
