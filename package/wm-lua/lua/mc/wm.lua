---@meta mc.wm

--- mc.wm — LuaLS type definitions for the `require("mc.wm")` module. The module itself is
--- **host-provided**: the embedding app registers it in C (uniwm's `libuniwm-lua` calls
--- `mc_wm_lua_open`, parking it in `package.loaded["mc.wm"]`) — there is no standalone
--- loadable. This file carries only the types (editor-only; not loaded at runtime).

---@class mcwm.Size
---@field width integer
---@field height integer

---@class mcwm.Position
---@field x integer
---@field y integer

---@class mcwm.Rect
---@field x integer
---@field y integer
---@field width integer
---@field height integer

---@alias mcwm.Area "window"|"decorated"|"drawable"
---@alias mcwm.WindowState "normal"|"minimized"|"maximized"|"fullscreen"

--- Event type names — serialized `GROUP.TYPE` form (as `mc_wm_event_type_str` emits /
--- `mc_wm_event_type_from_str` accepts). `"WM.NONE"` matches every event (but prefer `nil`
--- in `WM:on_event`). Registered user events use `"<group>.<event>"` (e.g. `"DEMO.PING"`,
--- `"UNIWM.WINDOW_FOCUSED"`); `string` is included so those (and any custom name) are accepted
--- while the built-ins still offer completion.
---@alias mcwm.EventType
---| string
---| "WM.NONE"
---| "WM.GENERIC.RAW"
---| "WM.WINDOW.READY"
---| "WM.WINDOW.RESIZED"
---| "WM.WINDOW.MOVED"
---| "WM.WINDOW.REDRAW_REQUESTED"
---| "WM.WINDOW.CLOSE_REQUESTED"
---| "WM.WINDOW.STATE_CHANGED"
---| "WM.WINDOW.FOCUS_GAINED"
---| "WM.WINDOW.FOCUS_LOST"
---| "WM.WINDOW.MOUSE_MOVED"
---| "WM.WINDOW.MOUSE_DOWN"
---| "WM.WINDOW.MOUSE_UP"
---| "WM.WINDOW.MOUSE_CLICK"
---| "WM.WINDOW.MOUSE_ENTER"
---| "WM.WINDOW.MOUSE_LEAVE"
---| "WM.WINDOW.MOUSE_WHEEL"
---| "WM.WINDOW.KEY_DOWN"
---| "WM.WINDOW.KEY_UP"
---| "WM.WINDOW.TEXT_INPUT"
---| "WM.WINDOW.PASTE_TEXT"
---| "WM.GLOBAL.KEY_DOWN"
---| "WM.GLOBAL.KEY_UP"
---| "WM.GLOBAL.MOUSE_MOVED"
---| "WM.GLOBAL.MOUSE_DOWN"
---| "WM.GLOBAL.MOUSE_UP"
---| "WM.GLOBAL.MOUSE_WHEEL"

---@class mcwm.WindowOpts
---@field title? string
---@field size? mcwm.Size
---@field position? mcwm.Position
---@field state? mcwm.WindowState

--- Events passed to `WM:on_event` callbacks are a tagged union discriminated by
--- `type`: narrow on `event.type` and each variant exposes exactly the fields it
--- carries. `window` is resolved lazily on first access (see `WM:on_event`); it is
--- absent for global events (`GLOBAL_*`).

--- `WINDOW_READY` / `WINDOW_REDRAW_REQUESTED` / `WINDOW_CLOSE_REQUESTED` /
--- `FOCUS_GAINED` / `FOCUS_LOST` — only the window.
---@class mcwm.Event.Window
---@field type "WM.WINDOW.READY"|"WM.WINDOW.REDRAW_REQUESTED"|"WM.WINDOW.CLOSE_REQUESTED"|"WM.WINDOW.FOCUS_GAINED"|"WM.WINDOW.FOCUS_LOST"
---@field window mcwm.Window

---@class mcwm.Event.WindowResized
---@field type "WM.WINDOW.RESIZED"
---@field window mcwm.Window
---@field new_size mcwm.Size

---@class mcwm.Event.WindowMoved
---@field type "WM.WINDOW.MOVED"
---@field window mcwm.Window
---@field new_position mcwm.Position

---@class mcwm.Event.WindowStateChanged
---@field type "WM.WINDOW.STATE_CHANGED"
---@field window mcwm.Window
---@field state mcwm.WindowState

--- `MOUSE_MOVED` / `MOUSE_ENTER` / `MOUSE_LEAVE`.
---@class mcwm.Event.MouseMove
---@field type "WM.WINDOW.MOUSE_MOVED"|"WM.WINDOW.MOUSE_ENTER"|"WM.WINDOW.MOUSE_LEAVE"
---@field window mcwm.Window
---@field position mcwm.Position

--- `MOUSE_DOWN` / `MOUSE_UP`.
---@class mcwm.Event.MouseButton
---@field type "WM.WINDOW.MOUSE_DOWN"|"WM.WINDOW.MOUSE_UP"
---@field window mcwm.Window
---@field position mcwm.Position
---@field button string

---@class mcwm.Event.MouseWheel
---@field type "WM.WINDOW.MOUSE_WHEEL"
---@field window mcwm.Window
---@field position mcwm.Position
---@field up integer
---@field right integer

--- `KEY_DOWN` / `KEY_UP`.
---@class mcwm.Event.Key
---@field type "WM.WINDOW.KEY_DOWN"|"WM.WINDOW.KEY_UP"
---@field window mcwm.Window
---@field key string

--- `TEXT_INPUT` / `PASTE_TEXT`.
---@class mcwm.Event.Text
---@field type "WM.WINDOW.TEXT_INPUT"|"WM.WINDOW.PASTE_TEXT"
---@field window mcwm.Window
---@field text string

--- `GLOBAL_KEY_DOWN` / `GLOBAL_KEY_UP`.
---@class mcwm.Event.GlobalKey
---@field type "WM.GLOBAL.KEY_DOWN"|"WM.GLOBAL.KEY_UP"
---@field key string

---@class mcwm.Event.GlobalMouseMove
---@field type "WM.GLOBAL.MOUSE_MOVED"
---@field position mcwm.Position

--- `GLOBAL_MOUSE_DOWN` / `GLOBAL_MOUSE_UP`.
---@class mcwm.Event.GlobalMouseButton
---@field type "WM.GLOBAL.MOUSE_DOWN"|"WM.GLOBAL.MOUSE_UP"
---@field position mcwm.Position
---@field button string

---@class mcwm.Event.GlobalMouseWheel
---@field type "WM.GLOBAL.MOUSE_WHEEL"
---@field position mcwm.Position
---@field up integer
---@field right integer

--- `NONE` / `RAW` / `MOUSE_CLICK` — no payload fields.
---@class mcwm.Event.Other
---@field type "WM.NONE"|"WM.GENERIC.RAW"|"WM.WINDOW.MOUSE_CLICK"

--- User-defined / libuniwm events (registered via `WM:register_events`, or libuniwm's
--- `UNIWM.*`) — `type` is the custom `GROUP.NAME` and the payload carries whatever fields
--- were sent (e.g. `window_id`, `window` for the `UNIWM.WINDOW_*` events). Arbitrary fields
--- are allowed with no diagnostic; the built-in variants above still drive `type` completion.
---@class mcwm.Event.User
---@field type string
---@field window? mcwm.Window
---@field window_id? integer
---@field [string] any

---@alias mcwm.Event
---| mcwm.Event.Window
---| mcwm.Event.WindowResized
---| mcwm.Event.WindowMoved
---| mcwm.Event.WindowStateChanged
---| mcwm.Event.MouseMove
---| mcwm.Event.MouseButton
---| mcwm.Event.MouseWheel
---| mcwm.Event.Key
---| mcwm.Event.Text
---| mcwm.Event.GlobalKey
---| mcwm.Event.GlobalMouseMove
---| mcwm.Event.GlobalMouseButton
---| mcwm.Event.GlobalMouseWheel
---| mcwm.Event.Other
---| mcwm.Event.User

---@class mcwm.Window
local Window = {}

---@param title string
---@return mcwm.Window self
function Window:set_title(title) end

---@param size mcwm.Size
---@param area? mcwm.Area # default "window"
---@return mcwm.Window self
function Window:set_size(size, area) end

---@param position mcwm.Position
---@param area? mcwm.Area # default "window"
---@return mcwm.Window self
function Window:set_position(position, area) end

---@param rect mcwm.Rect
---@param area? mcwm.Area # default "window"
---@return mcwm.Window self
function Window:set_rect(rect, area) end

---@param state mcwm.WindowState
---@return mcwm.Window self
function Window:set_state(state) end

---@param area? mcwm.Area # default "window"
---@return mcwm.Size
function Window:get_size(area) end

---@param area? mcwm.Area # default "window"
---@return mcwm.Position
function Window:get_position(area) end

---@param area? mcwm.Area # default "window"
---@return mcwm.Rect
function Window:get_rect(area) end

---@return string
function Window:get_title() end

---@return mcwm.WindowState
function Window:get_state() end

---@return boolean
function Window:is_alive() end

--- Whether this is a shell/system window (taskbar, desktop, cloaked UWP
--- CoreWindow, tool window) rather than a user application window.
---@return boolean
function Window:is_system() end

--- Politely ask the window to close (foreign: WM_CLOSE; managed: destroy).
function Window:close() end

--- Give the window input focus / bring it to the foreground.
---@return mcwm.Window self
function Window:focus() end

--- Destroy a window this WM owns (created via `WM:create_window`).
function Window:destroy() end

--- A reference to the shared window manager. The host (e.g. uniwm) owns the WM and
--- runs the event loop; scripts only ever hold references — there is intentionally no
--- `poll_event` here. Operations raise once the owner has destroyed the WM.
---@class mcwm.WM
local WM = {}

---@param opts? mcwm.WindowOpts
---@return mcwm.Window
function WM:create_window(opts) end

---@return mcwm.Window? # nil if there is no focused window
function WM:get_focused_window() end

---@return mcwm.Window? # nil if there is no hovered window
function WM:get_hovered_window() end

---@return mcwm.Window[]
function WM:get_all_windows() end

---@class mcwm.Subscription
local Subscription = {}

--- Stop receiving events for this subscription.
function Subscription:unsubscribe() end

--- Register a callback for events. `event_type` is a serialized `GROUP.TYPE` event name
--- (e.g. `"WM.GLOBAL.KEY_DOWN"`) or `nil` to match every event. The callback receives the
--- event as a table (see `mcwm.Event`); its `window` field is resolved lazily on
--- first access. The subscription stays active until `:unsubscribe()` (or process exit) —
--- you need not hold the returned handle to keep receiving events; keep it only if you want
--- to `:unsubscribe()`. Callbacks fire from the host's event loop, which decides which
--- events to dispatch.
---@param event_type? mcwm.EventType
---@param callback fun(event: mcwm.Event)
---@return mcwm.Subscription
function WM:on_event(event_type, callback) end

---@class mcwm.UserEventDef
---@field name string

--- Register a user event subgroup. `group` is the group name used directly (no prefix —
--- e.g. `"DEMO"`); `events` is an ordered list of `{name=...}` defs whose full event names
--- become `<group>.<name>` (e.g. `"DEMO.ALERT"`). Errors if the subgroup is already registered.
---@param group string # e.g. "DEMO"
---@param events mcwm.UserEventDef[]
function WM:register_events(group, events) end

--- Enqueue a user event for delivery (FIFO, ahead of target events). `name` is a full
--- registered user-event name (`<group>.<name>`, e.g. `"DEMO.MESSAGE"`); `data` is an
--- optional table serialized into the event payload (JSON). Errors if `name` is not a
--- registered event.
---@param name string # e.g. "DEMO.MESSAGE"
---@param data? table
function WM:send_event(name, data) end

--- Release this reference to the WM. Does not tear down the WM itself — the owner
--- keeps it alive until the last reference is dropped.
function WM:destroy() end

---@class mcwm
local mcwm = {}

--- Get a reference to the shared window manager (lazily created on first use if no
--- host has created one).
---@param impl? string # require a specific backend, e.g. "WIN32"
---@return mcwm.WM
function mcwm.resolve(impl) end

return mcwm
