---@meta mc.core

--- mc.core — LuaLS type definitions for the `require("mc.core")` module (Lua bindings for
--- mc's `core` functionality). Like `mc.wm`, the module is **host-provided**: the embedding
--- app registers it in C (`mc_core_lua_open`, parking it in `package.loaded["mc.core"]`).
--- This file carries only the types (editor-only; not loaded at runtime).

--- An opaque handle to a task created on a scheduler (`Sched:task`). Cooperative: its
--- handler is called repeatedly while it returns `"continue"`, paused while it returns
--- `"suspend"`, and finished when it returns `"done"` (or `nil`). The task is already
--- scheduled when created — the handle is only needed to keep it alive.
---@class mc.core.Task
local Task = {}

--- A cooperative scheduler (`mc_sched`). Create tasks on it, then drive it with
--- `:run()` (to completion) or `:step()` (one iteration).
---@class mc.core.Sched
local Sched = {}

--- Create and schedule a task whose handler is `fn`. `fn` returns the next status:
--- `"continue"` (call again), `"suspend"` (pause), or `"done"`/`nil` (finish).
--- The task runs on the next `:run()`/`:step()`; pass `delay_ms` to defer its first
--- run by that many milliseconds.
---@param fn fun(): ("done"|"continue"|"suspend")?
---@param delay_ms? integer
---@return mc.core.Task
function Sched:task(fn, delay_ms) end

--- Run the scheduler until all tasks are done.
function Sched:run() end

--- Run one scheduler iteration; returns the resulting status:
--- `"done"` (no work left) / `"suspend"` / `"continue"`.
---@return "done"|"suspend"|"continue"
function Sched:step() end

---@class mc.core
local M = {}

--- Create a new scheduler.
---@return mc.core.Sched
function M.sched() end

return M
