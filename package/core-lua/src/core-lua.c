#include <mc/core-lua/core-lua.h>

#include <mc/sched.h>
#include <mc/time.h>

#include <lua.h>
#include <lauxlib.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SCHED_MT "mc.core.sched"
#define TASK_MT  "mc.core.task"

typedef struct LuaSched {
    MC_Sched *sched;
} LuaSched;

typedef struct LuaTask {
    MC_Task *task;
} LuaTask;

typedef struct LuaTaskCtx {
    lua_State *L;
    int fn_ref;
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

static MC_TaskStatus lua_task_do_some(MC_Task *task) {
    LuaTaskCtx *ctx = mc_task_data(task, NULL);
    if (ctx->fn_ref == LUA_NOREF) {
        return MC_TASK_DONE;
    }

    lua_State *L = ctx->L;
    lua_rawgeti(L, LUA_REGISTRYINDEX, ctx->fn_ref);

    MC_TaskStatus status = MC_TASK_DONE;
    if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
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
        ctx->fn_ref = LUA_NOREF;
    }

    return status;
}

static int l_sched_task(lua_State *L) {
    LuaSched *s = luaL_checkudata(L, 1, SCHED_MT);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    lua_Integer delay_ms = luaL_optinteger(L, 3, 0);
    if (s->sched == NULL) {
        return luaL_error(L, "mc.core: scheduler is destroyed");
    }

    lua_pushvalue(L, 2);
    int fn_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    LuaTaskCtx ctx = { .L = L, .fn_ref = fn_ref };
    MC_Task *task = NULL;
    if (mc_task_new(s->sched, &task, lua_task_do_some, sizeof(ctx), &ctx) != MCE_OK) {
        luaL_unref(L, LUA_REGISTRYINDEX, fn_ref);
        return luaL_error(L, "mc.core: failed to create task");
    }

    if (delay_ms > 0) {
        MC_Time delay = {
            .sec = (uint64_t)(delay_ms / MC_MSEC_IN_SEC),
            .nsec = (uint64_t)(delay_ms % MC_MSEC_IN_SEC) * (MC_NSEC_IN_SEC / MC_MSEC_IN_SEC),
        };
        if (mc_task_delay(task, delay) != MCE_OK) {
            mc_task_unref(task);
            luaL_unref(L, LUA_REGISTRYINDEX, fn_ref);
            return luaL_error(L, "mc.core: failed to delay task");
        }
    }

    if (mc_task_run(task) != MCE_OK) {
        mc_task_unref(task);
        luaL_unref(L, LUA_REGISTRYINDEX, fn_ref);
        return luaL_error(L, "mc.core: failed to schedule task");
    }

    LuaTask *t = lua_newuserdatauv(L, sizeof(*t), 0);
    t->task = task;
    luaL_setmetatable(L, TASK_MT);
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

static int l_sched_gc(lua_State *L) {
    LuaSched *s = luaL_checkudata(L, 1, SCHED_MT);
    if (s->sched != NULL) {
        mc_sched_delete(s->sched);
        s->sched = NULL;
    }

    return 0;
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
    luaL_setmetatable(L, SCHED_MT);

    if (mc_sched_new(&s->sched) != MCE_OK) {
        return luaL_error(L, "mc.core: failed to create scheduler");
    }

    return 1;
}

static void register_class(lua_State *L, const char *name, const luaL_Reg *methods) {
    luaL_newmetatable(L, name);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, methods, 0);
    lua_pop(L, 1);
}

int mc_core_lua_module(lua_State *L) {
    static const luaL_Reg sched_methods[] = {
        { "task", l_sched_task },
        { "run", l_sched_run },
        { "step", l_sched_step },
        { "__gc", l_sched_gc },
        { NULL, NULL },
    };

    static const luaL_Reg task_methods[] = {
        { "__gc", l_task_gc },
        { NULL, NULL },
    };

    static const luaL_Reg module[] = {
        { "sched", l_sched_new },
        { NULL, NULL },
    };

    register_class(L, SCHED_MT, sched_methods);
    register_class(L, TASK_MT, task_methods);

    luaL_newlib(L, module);
    return 1;
}

void mc_core_lua_open(lua_State *L) {
    luaL_getsubtable(L, LUA_REGISTRYINDEX, LUA_LOADED_TABLE);
    mc_core_lua_module(L);
    lua_setfield(L, -2, "mc.core");
    lua_pop(L, 1);
}
