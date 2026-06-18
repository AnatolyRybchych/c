---@meta mc.core

--- mc.core — LuaLS type definitions for the `require("mc.core")` module (Lua bindings for
--- mc's `core` functionality). Like `mc.wm`, the module is **host-provided**: the embedding
--- app registers it in C (`mc_core_lua_open`, parking it in `package.loaded["mc.core"]`).
--- This file carries only the types (editor-only; not loaded at runtime).

--- A task created on a scheduler (`Sched:task`). Cooperative: its handler is called
--- repeatedly while it returns `"continue"`, paused while it returns `"suspend"`, and
--- finished when it returns `"done"` (or `nil`). A freshly created task is **not**
--- scheduled — call `:run()`, `:schedule(timeout)`, or `:run_after(...)` to start it.
---@class mc.core.Task
---@field sched mc.core.Sched the scheduler this task belongs to
---@field data any the data passed as the second argument to `Sched:task`
local Task = {}

--- Schedule the task to run as soon as possible. Returns self for chaining.
---@return mc.core.Task self
function Task:run() end

--- Schedule the task to run after `timeout` milliseconds (call on a not-yet-started
--- task). Returns self for chaining.
---@param timeout integer milliseconds
---@return mc.core.Task self
function Task:schedule(timeout) end

--- Schedule the task to run only after every given task has finished. Returns self
--- for chaining.
---@param ... mc.core.Task
---@return mc.core.Task self
function Task:run_after(...) end

--- Defer the task's next activation by `delay` milliseconds (e.g. called from the
--- handler to re-run later). Returns self for chaining.
---@param delay integer milliseconds
---@return mc.core.Task self
function Task:delay(delay) end

--- The task's current status: `"done"`, `"suspend"` (waiting for its scheduled time),
--- or `"continue"` (runnable / not yet finished).
---@return "done"|"suspend"|"continue"
function Task:status() end

--- A cooperative scheduler (`mc_sched`). Create tasks on it, then drive it with
--- `:run()` (to completion) or `:step()` (one iteration).
---@class mc.core.Sched
local Sched = {}

--- Create a task whose handler is `fn`. `fn` receives the task itself and returns the
--- next status: `"continue"` (call again), `"suspend"` (pause), or `"done"`/`nil`
--- (finish). The task is **not** scheduled until `:run()`/`:schedule()`/`:run_after()`.
--- `data` is associated with the task and readable as `task.data`.
---@param fn fun(task: mc.core.Task): ("done"|"continue"|"suspend")?
---@param data? any
---@return mc.core.Task
function Sched:task(fn, data) end

--- Run the scheduler until all tasks are done.
function Sched:run() end

--- Run one scheduler iteration; returns the resulting status:
--- `"done"` (no work left) / `"suspend"` / `"continue"`.
---@return "done"|"suspend"|"continue"
function Sched:step() end

--- Drive the scheduler until every given task has finished, or `timeout`
--- milliseconds elapse (`nil` = wait forever). Returns `true` if all tasks
--- completed, `false` if it returned due to the timeout.
---@param timeout integer|nil milliseconds, or nil for no timeout
---@param ... mc.core.Task
---@return boolean completed
function Sched:wait(timeout, ...) end

---@class mc.core
local M = {}

--- Create a new scheduler.
---@return mc.core.Sched
function M.sched() end

return M
