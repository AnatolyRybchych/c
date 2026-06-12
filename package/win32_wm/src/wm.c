#include <mc/win32_wm/wm.h>
#include <mc/win32_wm/graphics.h>
#include <mc/win32_wm/key.h>

#include <mc/data/stream.h>
#include <mc/data/alloc.h>
#include <mc/data/str.h>
#include <mc/data/list.h>
#include <mc/util/minmax.h>

#include <windowsx.h>
#include <dwmapi.h>

#include <string.h>
#include <wchar.h>

#ifndef DWM_CLOAKED_APP
#define DWM_CLOAKED_APP 0x00000001
#endif

#define LOG(FMT, ...) mc_fmt(wm->log, "[WM][WIN32] " FMT "\n", ##__VA_ARGS__)

#define WINDOW_CLASS_NAME "mc_win32_wm_window"
#define EVENT_QUEUE_CAP 1024
#define ENUM_WINDOWS_CAP 512

struct rawevent{
    HWND hwnd;
    UINT message;
    WPARAM wparam;
    LPARAM lparam;
};

struct MC_TargetForeignWindow{
    HWND hwnd;
};

struct identity_node{
    struct identity_node *next;
    MC_TargetResolvedIdentity resolved;
    struct MC_TargetForeignWindow foreign_window;
};

struct MC_TargetWM{
    MC_Stream *log;
    MC_Alloc *arena;
    HINSTANCE instance;
    ATOM window_class;

    HHOOK keyboard_hook;
    HHOOK mouse_hook;
    bool (*keyboard_suppress)(MC_TargetWM *wm, MC_Key key, bool down);
    bool (*mouse_suppress)(MC_TargetWM *wm, UINT message, int x, int y);

    struct identity_node *identities;

    struct rawevent queue[EVENT_QUEUE_CAP];
    unsigned queue_head;
    unsigned queue_count;
};

struct MC_TargetWMWindow{
    MC_TargetWM *wm;
    HWND hwnd;
    bool mouse_inside;
    bool fullscreen;
    LONG saved_style;
    WINDOWPLACEMENT saved_placement;
};

struct MC_TargetWMEvent{
    struct rawevent raw;
};

static struct MC_TargetWM *window_class_owner;
static struct MC_TargetWM *global_hook_wm;

static MC_Error init(struct MC_TargetWM *wm, MC_Stream *log, MC_Alloc *arena);
static void destroy(struct MC_TargetWM *wm);
static MC_Error init_window(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window);
static void destroy_window(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window);
static MC_Error create_window_graphic(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, struct MC_Graphics **g);
static MC_Error set_window_title(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_Str title);
static MC_Error set_window_state(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_WMWindowState state);
static MC_Error set_window_rect(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_WMArea area, MC_Rect2IU rect);
static MC_Error focus_window(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window);
static MC_Error get_window_rect(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_WMArea area, MC_Rect2IU *rect);
static MC_Error get_window_state(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_WMWindowState *state);
static MC_Error get_window_title(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, char *utf8, size_t cap, size_t *len);
static bool poll_event(struct MC_TargetWM *wm, struct MC_TargetWMEvent *event);
static unsigned translate_event(struct MC_TargetWM *wm, const struct MC_TargetWMEvent *event, MC_TargetIndication indications[MC_WM_MAX_INDICATIONS_PER_EVENT]);

static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
static void enqueue(struct MC_TargetWM *wm, HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
static void set_fullscreen(struct MC_TargetWMWindow *window, bool on);
static MC_MouseButton get_mouse_button(UINT message, WPARAM wparam);
static MC_Key get_key(WPARAM vk);
static struct MC_TargetWMWindow *window_of(HWND hwnd);
static bool is_our_window(HWND hwnd);
static MC_Error read_clipboard(struct MC_TargetWM *wm, HWND hwnd, MC_Str *out);
static MC_Error request_events(struct MC_TargetWM *wm, MC_WMEvents events);
static MC_Error get_focused_window(struct MC_TargetWM *wm, uint64_t *identity);
static MC_Error get_hovered_window(struct MC_TargetWM *wm, uint64_t *identity);
static MC_Error get_all_windows(struct MC_TargetWM *wm, uint64_t **identities, size_t *count);
static BOOL CALLBACK enum_windows_proc(HWND hwnd, LPARAM lparam);
static MC_Error resolve_temporary_identity(struct MC_TargetWM *wm, uint64_t identity, MC_TargetResolvedIdentity *out);
static void heartbeat(struct MC_TargetWM *wm);
static MC_Error close_foreign_window(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window);
static MC_Error set_foreign_window_title(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window, MC_Str title);
static MC_Error set_foreign_window_rect(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window, MC_WMArea area, MC_Rect2IU rect);
static MC_Error set_foreign_window_state(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window, MC_WMWindowState state);
static MC_Error focus_foreign_window(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window);
static MC_Error get_foreign_window_title(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window, char *utf8, size_t cap, size_t *len);
static MC_Error get_foreign_window_rect(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window, MC_WMArea area, MC_Rect2IU *rect);
static MC_Error get_foreign_window_state(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window, MC_WMWindowState *state);
static MC_Error is_foreign_window_system(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window, bool *out);
static unsigned translate_global_event(const struct rawevent *e, MC_TargetIndication indications[MC_WM_MAX_INDICATIONS_PER_EVENT]);
static void enqueue_global(struct MC_TargetWM *wm, UINT message, WPARAM wparam, POINT point);
static LRESULT CALLBACK keyboard_hook_proc(int code, WPARAM wparam, LPARAM lparam);
static LRESULT CALLBACK mouse_hook_proc(int code, WPARAM wparam, LPARAM lparam);
static LPARAM pack_point(POINT point);
static MC_Vec2i unpack_point(LPARAM lparam);

static MC_Error hwnd_set_title(HWND hwnd, MC_Str title);
static MC_Error hwnd_get_title(HWND hwnd, char *utf8, size_t cap, size_t *len);
static MC_Error hwnd_set_rect(HWND hwnd, MC_WMArea area, MC_Rect2IU rect);
static MC_Error hwnd_get_rect(HWND hwnd, MC_WMArea area, MC_Rect2IU *rect);
static MC_Error hwnd_set_state(HWND hwnd, MC_WMWindowState state);
static MC_Error hwnd_get_state(HWND hwnd, MC_WMWindowState *state);
static MC_Error hwnd_close(HWND hwnd);
static MC_Error hwnd_is_system(HWND hwnd, bool *out);
static MC_Error hwnd_focus(HWND hwnd);


static MC_WMVtab vtab = {
    .name = "WIN32",
    .wm_size = sizeof(struct MC_TargetWM),
    .window_size = sizeof(struct MC_TargetWMWindow),
    .foreign_window_size = sizeof(struct MC_TargetForeignWindow),
    .event_size = sizeof(struct MC_TargetWMEvent),

    .init = init,
    .destroy = destroy,

    .init_window = init_window,
    .destroy_window = destroy_window,
    .create_window_graphic = create_window_graphic,

    .set_window_title = set_window_title,
    .set_window_state = set_window_state,
    .set_window_rect = set_window_rect,
    .focus_window = focus_window,
    .get_window_title = get_window_title,
    .get_window_state = get_window_state,
    .get_window_rect = get_window_rect,

    .poll_event = poll_event,
    .translate_event = translate_event,

    .request_events = request_events,

    .get_focused_window = get_focused_window,
    .get_hovered_window = get_hovered_window,
    .get_all_windows = get_all_windows,
    .resolve_temporary_identity = resolve_temporary_identity,
    .heartbeat = heartbeat,

    .close_foreign_window = close_foreign_window,
    .set_foreign_window_title = set_foreign_window_title,
    .set_foreign_window_state = set_foreign_window_state,
    .set_foreign_window_rect = set_foreign_window_rect,
    .focus_foreign_window = focus_foreign_window,

    .get_foreign_window_title = get_foreign_window_title,
    .get_foreign_window_state = get_foreign_window_state,
    .get_foreign_window_rect = get_foreign_window_rect,
    .is_foreign_window_system = is_foreign_window_system,
};

const MC_WMVtab *mc_win32_wm_vtab = &vtab;

HWND mc_wm_win32_window_get_hwnd(MC_TargetWMWindow *window){
    return window->hwnd;
}

void mc_wm_win32_set_keyboard_suppress(MC_TargetWM *wm,
    bool (*suppress)(MC_TargetWM *wm, MC_Key key, bool down)){
    wm->keyboard_suppress = suppress;
}

void mc_wm_win32_set_mouse_suppress(MC_TargetWM *wm,
    bool (*suppress)(MC_TargetWM *wm, UINT message, int x, int y)){
    wm->mouse_suppress = suppress;
}

static MC_Error init(struct MC_TargetWM *wm, MC_Stream *log, MC_Alloc *arena){
    wm->log = log;
    wm->arena = arena;
    wm->instance = GetModuleHandle(NULL);
    wm->queue_head = 0;
    wm->queue_count = 0;

    if(window_class_owner != NULL){
        LOG("window class is already registered by another window manager");
        return MCE_BUSY;
    }

    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(wc);
    wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = window_proc;
    wc.hInstance = wm->instance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = WINDOW_CLASS_NAME;

    wm->window_class = RegisterClassEx(&wc);
    if(wm->window_class == 0){
        LOG("RegisterClassEx failed (%lu)", (unsigned long)GetLastError());
        return MCE_NOT_SUPPORTED;
    }

    window_class_owner = wm;

    return MCE_OK;
}

static void destroy(struct MC_TargetWM *wm){
    if(wm->keyboard_hook){
        UnhookWindowsHookEx(wm->keyboard_hook);
        wm->keyboard_hook = NULL;
    }

    if(wm->mouse_hook){
        UnhookWindowsHookEx(wm->mouse_hook);
        wm->mouse_hook = NULL;
    }

    if(global_hook_wm == wm){
        global_hook_wm = NULL;
    }

    if(window_class_owner == wm){
        UnregisterClass(WINDOW_CLASS_NAME, wm->instance);
        window_class_owner = NULL;
    }
}

static MC_Error init_window(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window){
    window->wm = wm;
    window->hwnd = NULL;
    window->mouse_inside = false;
    window->fullscreen = false;

    HWND hwnd = CreateWindowEx(0, WINDOW_CLASS_NAME, "", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, wm->instance, window);

    if(hwnd == NULL){
        LOG("CreateWindowEx failed (%lu)", (unsigned long)GetLastError());
        return MCE_NOT_SUPPORTED;
    }

    window->hwnd = hwnd;

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    return MCE_OK;
}

static void destroy_window(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window){
    (void)wm;
    if(window->hwnd){
        DestroyWindow(window->hwnd);
        window->hwnd = NULL;
    }
}

static MC_Error close_foreign_window(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window){
    (void)wm;
    return hwnd_close(window->hwnd);
}

static MC_Error create_window_graphic(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, struct MC_Graphics **g){
    (void)wm;
    return mc_win32_graphics_init(g, window->hwnd);
}

static MC_Error hwnd_set_title(HWND hwnd, MC_Str title){
    char utf8[512];
    size_t len = MIN(mc_str_len(title), sizeof(utf8) - 1);
    memcpy(utf8, title.beg, len);
    utf8[len] = '\0';

    wchar_t wide[512];
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8, (int)len, wide, (int)(sizeof(wide) / sizeof(*wide)) - 1);
    wide[wlen > 0 ? wlen : 0] = L'\0';

    return SetWindowTextW(hwnd, wide) ? MCE_OK : MCE_UNKNOWN;
}

static MC_Error hwnd_get_title(HWND hwnd, char *utf8, size_t cap, size_t *len){
    wchar_t wide[256];
    int wlen = GetWindowTextW(hwnd, wide, (int)(sizeof(wide) / sizeof(*wide)));

    int n = WideCharToMultiByte(CP_UTF8, 0, wide, wlen, utf8, (int)cap, NULL, NULL);
    *len = n > 0 ? (size_t)n : 0;

    return MCE_OK;
}

static void hwnd_invisible_margins(HWND hwnd, int *left, int *top, int *right, int *bottom){
    RECT window, frame;
    if(!GetWindowRect(hwnd, &window) ||
       FAILED(DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &frame, sizeof(frame)))){
        *left = *top = *right = *bottom = 0;
        return;
    }

    *left = frame.left - window.left;
    *top = frame.top - window.top;
    *right = window.right - frame.right;
    *bottom = window.bottom - frame.bottom;
}

static MC_Error hwnd_get_rect(HWND hwnd, MC_WMArea area, MC_Rect2IU *rect){
    RECT r;
    switch(area){
    case MC_WM_AREA_WINDOW:
        if(!GetWindowRect(hwnd, &r)){
            return MCE_UNKNOWN;
        }
        break;
    case MC_WM_AREA_DECORATED:
        if(FAILED(DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &r, sizeof(r))) && !GetWindowRect(hwnd, &r)){
            return MCE_UNKNOWN;
        }
        break;
    case MC_WM_AREA_DRAWABLE: {
        POINT origin = {0, 0};
        if(!GetClientRect(hwnd, &r) || !ClientToScreen(hwnd, &origin)){
            return MCE_UNKNOWN;
        }
        r.left = origin.x;
        r.top = origin.y;
        r.right = origin.x + (r.right - r.left);
        r.bottom = origin.y + (r.bottom - r.top);
        break;
    }
    default:
        return MCE_NOT_SUPPORTED;
    }

    *rect = (MC_Rect2IU){
        .x = r.left,
        .y = r.top,
        .width = (unsigned)(r.right - r.left),
        .height = (unsigned)(r.bottom - r.top),
    };

    return MCE_OK;
}

static MC_Error hwnd_set_rect(HWND hwnd, MC_WMArea area, MC_Rect2IU rect){
    int left = 0, top = 0, right = 0, bottom = 0;
    switch(area){
    case MC_WM_AREA_WINDOW:
        break;
    case MC_WM_AREA_DECORATED:
        hwnd_invisible_margins(hwnd, &left, &top, &right, &bottom);
        break;
    case MC_WM_AREA_DRAWABLE: {
        RECT window, client;
        POINT client_origin = {0, 0};
        if(!GetWindowRect(hwnd, &window) || !ClientToScreen(hwnd, &client_origin) || !GetClientRect(hwnd, &client)){
            return MCE_UNKNOWN;
        }
        left = client_origin.x - window.left;
        top = client_origin.y - window.top;
        right = window.right - (client_origin.x + (client.right - client.left));
        bottom = window.bottom - (client_origin.y + (client.bottom - client.top));
        break;
    }
    default:
        return MCE_NOT_SUPPORTED;
    }

    BOOL ok = SetWindowPos(hwnd, NULL,
        rect.x - left, rect.y - top,
        (int)rect.width + left + right, (int)rect.height + top + bottom,
        SWP_NOZORDER | SWP_NOACTIVATE);

    return ok ? MCE_OK : MCE_UNKNOWN;
}

static MC_Error hwnd_set_state(HWND hwnd, MC_WMWindowState state){
    MC_WMWindowState current;
    if(hwnd_get_state(hwnd, &current) == MCE_OK && current == state){
        return MCE_OK;
    }

    switch(state){
    case MC_WM_WINDOW_STATE_NORMAL:
        ShowWindow(hwnd, SW_SHOWNORMAL);
        return MCE_OK;
    case MC_WM_WINDOW_STATE_MINIMIZED:
        ShowWindow(hwnd, SW_MINIMIZE);
        return MCE_OK;
    case MC_WM_WINDOW_STATE_MAXIMIZED:
        ShowWindow(hwnd, SW_MAXIMIZE);
        return MCE_OK;
    default:
        return MCE_NOT_SUPPORTED;
    }
}

static MC_Error hwnd_close(HWND hwnd){
    PostMessage(hwnd, WM_CLOSE, 0, 0);
    return MCE_OK;
}

static MC_Error hwnd_is_system(HWND hwnd, bool *out){
    *out = true;

    if(hwnd == NULL || !IsWindow(hwnd)){
        return MCE_OK;
    }

    DWORD cloaked = 0;
    if(SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked)))
        && (cloaked & DWM_CLOAKED_APP)){
        return MCE_OK;
    }

    if(GetWindowLongPtrW(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW){
        return MCE_OK;
    }

    static const wchar_t *const system_classes[] = {
        L"Progman",
        L"WorkerW",
        L"Shell_TrayWnd",
        L"Shell_SecondaryTrayWnd",
        L"Windows.UI.Core.CoreWindow",
    };

    wchar_t cls[128];
    int n = GetClassNameW(hwnd, cls, (int)(sizeof(cls) / sizeof(cls[0])));
    for(size_t i = 0; n > 0 && i < sizeof(system_classes) / sizeof(system_classes[0]); i++){
        if(wcscmp(cls, system_classes[i]) == 0){
            return MCE_OK;
        }
    }

    *out = false;
    return MCE_OK;
}

static MC_Error hwnd_focus(HWND hwnd){
    if(!IsWindow(hwnd)){
        return MCE_INVALID_INPUT;
    }

    if(IsIconic(hwnd)){
        ShowWindow(hwnd, SW_RESTORE);
    }

    DWORD self_thread = GetCurrentThreadId();
    HWND foreground = GetForegroundWindow();
    DWORD foreground_thread = foreground != NULL ? GetWindowThreadProcessId(foreground, NULL) : 0;
    DWORD target_thread = GetWindowThreadProcessId(hwnd, NULL);

    bool attached_foreground = foreground_thread != 0 && foreground_thread != self_thread;
    bool attached_target = target_thread != 0 && target_thread != self_thread && target_thread != foreground_thread;

    if(attached_foreground){
        AttachThreadInput(self_thread, foreground_thread, TRUE);
    }
    if(attached_target){
        AttachThreadInput(self_thread, target_thread, TRUE);
    }

    BringWindowToTop(hwnd);
    BOOL ok = SetForegroundWindow(hwnd);

    if(attached_target){
        AttachThreadInput(self_thread, target_thread, FALSE);
    }
    if(attached_foreground){
        AttachThreadInput(self_thread, foreground_thread, FALSE);
    }

    return ok ? MCE_OK : MCE_UNKNOWN;
}

static MC_Error hwnd_get_state(HWND hwnd, MC_WMWindowState *state){
    if(IsIconic(hwnd)){
        *state = MC_WM_WINDOW_STATE_MINIMIZED;
    }
    else if(IsZoomed(hwnd)){
        *state = MC_WM_WINDOW_STATE_MAXIMIZED;
    }
    else{
        *state = MC_WM_WINDOW_STATE_NORMAL;
    }

    return MCE_OK;
}

static MC_Error set_window_title(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_Str title){
    (void)wm;
    return hwnd_set_title(window->hwnd, title);
}

static MC_Error set_window_rect(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_WMArea area, MC_Rect2IU rect){
    (void)wm;
    return hwnd_set_rect(window->hwnd, area, rect);
}

static MC_Error get_window_rect(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_WMArea area, MC_Rect2IU *rect){
    (void)wm;
    return hwnd_get_rect(window->hwnd, area, rect);
}

static void set_fullscreen(struct MC_TargetWMWindow *window, bool on){
    if(on && !window->fullscreen){
        window->saved_style = GetWindowLong(window->hwnd, GWL_STYLE);
        window->saved_placement.length = sizeof(window->saved_placement);
        GetWindowPlacement(window->hwnd, &window->saved_placement);

        MONITORINFO mi = {0};
        mi.cbSize = sizeof(mi);
        HMONITOR monitor = MonitorFromWindow(window->hwnd, MONITOR_DEFAULTTONEAREST);
        if(GetMonitorInfo(monitor, &mi)){
            SetWindowLong(window->hwnd, GWL_STYLE, window->saved_style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(window->hwnd, HWND_TOP,
                mi.rcMonitor.left, mi.rcMonitor.top,
                mi.rcMonitor.right - mi.rcMonitor.left,
                mi.rcMonitor.bottom - mi.rcMonitor.top,
                SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }

        window->fullscreen = true;
    }
    else if(!on && window->fullscreen){
        SetWindowLong(window->hwnd, GWL_STYLE, window->saved_style);
        SetWindowPlacement(window->hwnd, &window->saved_placement);
        SetWindowPos(window->hwnd, NULL, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);

        window->fullscreen = false;
    }
}

static MC_Error set_window_state(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_WMWindowState state){
    switch(state){
    case MC_WM_WINDOW_STATE_NORMAL:
    case MC_WM_WINDOW_STATE_MAXIMIZED:
        set_fullscreen(window, false);
        return hwnd_set_state(window->hwnd, state);
    case MC_WM_WINDOW_STATE_MINIMIZED:
        return hwnd_set_state(window->hwnd, state);
    case MC_WM_WINDOW_STATE_FULLSCREEN:
        set_fullscreen(window, true);
        return MCE_OK;
    default:
        LOG("set_window_state: unsupported state %u", (unsigned)state);
        return MCE_INVALID_INPUT;
    }
}

static MC_Error focus_window(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window){
    (void)wm;
    return hwnd_focus(window->hwnd);
}

static MC_Error get_window_state(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_WMWindowState *state){
    (void)wm;
    if(window->fullscreen){
        *state = MC_WM_WINDOW_STATE_FULLSCREEN;
        return MCE_OK;
    }

    return hwnd_get_state(window->hwnd, state);
}

static MC_Error get_window_title(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, char *utf8, size_t cap, size_t *len){
    (void)wm;
    return hwnd_get_title(window->hwnd, utf8, cap, len);
}

static bool poll_event(struct MC_TargetWM *wm, struct MC_TargetWMEvent *event){
    while(wm->queue_count == 0){
        MSG msg;
        if(!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)){
            return false;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    event->raw = wm->queue[wm->queue_head];
    wm->queue_head = (wm->queue_head + 1) % EVENT_QUEUE_CAP;
    wm->queue_count--;

    return true;
}

static unsigned translate_event(struct MC_TargetWM *wm, const struct MC_TargetWMEvent *event, MC_TargetIndication indications[MC_WM_MAX_INDICATIONS_PER_EVENT]){
    const struct rawevent *e = &event->raw;

    if(e->hwnd == NULL){
        return translate_global_event(e, indications);
    }

    struct MC_TargetWMWindow *window = window_of(e->hwnd);

    switch(e->message){
    case WM_CLOSE:
        indications[0] = (MC_TargetIndication){
            .type = MC_WMIND_WINDOW_CLOSE_REQUESTED,
            .as.window_close_requested = {.window = window},
        };
        return 1;
    case WM_PAINT:
        indications[0] = (MC_TargetIndication){
            .type = MC_WMIND_WINDOW_REDRAW_REQUESTED,
            .as.redraw_requested = {.window = window},
        };
        return 1;
    case WM_MOVE:
        indications[0] = (MC_TargetIndication){
            .type = MC_WMIND_WINDOW_MOVED,
            .as.window_moved = {
                .window = window,
                .new_position = {.x = GET_X_LPARAM(e->lparam), .y = GET_Y_LPARAM(e->lparam)},
            },
        };
        return 1;
    case WM_SIZE: {
        unsigned n = 0;

        indications[n++] = (MC_TargetIndication){
            .type = MC_WMIND_WINDOW_RESIZED,
            .as.window_resized = {
                .window = window,
                .new_size = {.width = LOWORD(e->lparam), .height = HIWORD(e->lparam)},
            },
        };

        MC_WMWindowState state;
        bool has_state = true;
        switch(e->wparam){
        case SIZE_MINIMIZED: state = MC_WM_WINDOW_STATE_MINIMIZED; break;
        case SIZE_MAXIMIZED: state = MC_WM_WINDOW_STATE_MAXIMIZED; break;
        case SIZE_RESTORED:  state = MC_WM_WINDOW_STATE_NORMAL;    break;
        default:             has_state = false;                    break;
        }

        if(has_state){
            indications[n++] = (MC_TargetIndication){
                .type = MC_WMIND_WINDOW_STATE_CHANGED,
                .as.window_state_changed = {.window = window, .state = state},
            };
        }

        return n;
    }
    case WM_SETFOCUS:
        indications[0] = (MC_TargetIndication){
            .type = MC_WMIND_FOCUS_GAINED,
            .as.focus_gained = {.window = window},
        };
        return 1;
    case WM_KILLFOCUS:
        indications[0] = (MC_TargetIndication){
            .type = MC_WMIND_FOCUS_LOST,
            .as.focus_lost = {.window = window},
        };
        return 1;
    case WM_MOUSEMOVE: {
        unsigned n = 0;
        MC_Vec2i position = {.x = GET_X_LPARAM(e->lparam), .y = GET_Y_LPARAM(e->lparam)};

        if(window && !window->mouse_inside){
            window->mouse_inside = true;

            TRACKMOUSEEVENT tme = {sizeof(tme), TME_LEAVE, e->hwnd, 0};
            TrackMouseEvent(&tme);

            indications[n++] = (MC_TargetIndication){
                .type = MC_WMIND_MOUSE_ENTER,
                .as.mouse_enter = {.window = window},
            };
        }

        indications[n++] = (MC_TargetIndication){
            .type = MC_WMIND_MOUSE_MOVED,
            .as.mouse_moved = {.window = window, .position = position},
        };

        return n;
    }
    case WM_MOUSELEAVE:
        if(window){
            window->mouse_inside = false;
        }

        indications[0] = (MC_TargetIndication){
            .type = MC_WMIND_MOUSE_LEAVE,
            .as.mouse_leave = {.window = window},
        };
        return 1;
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_XBUTTONDOWN:
        indications[0] = (MC_TargetIndication){
            .type = MC_WMIND_MOUSE_DOWN,
            .as.mouse_down = {.window = window, .button = get_mouse_button(e->message, e->wparam)},
        };
        return 1;
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_XBUTTONUP:
        indications[0] = (MC_TargetIndication){
            .type = MC_WMIND_MOUSE_UP,
            .as.mouse_up = {.window = window, .button = get_mouse_button(e->message, e->wparam)},
        };
        return 1;
    case WM_MOUSEWHEEL:
        indications[0] = (MC_TargetIndication){
            .type = MC_WMIND_MOUSE_WHEEL,
            .as.mouse_wheel = {.window = window, .up = GET_WHEEL_DELTA_WPARAM(e->wparam) / WHEEL_DELTA, .right = 0},
        };
        return 1;
    case WM_MOUSEHWHEEL:
        indications[0] = (MC_TargetIndication){
            .type = MC_WMIND_MOUSE_WHEEL,
            .as.mouse_wheel = {.window = window, .up = 0, .right = GET_WHEEL_DELTA_WPARAM(e->wparam) / WHEEL_DELTA},
        };
        return 1;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN: {
        unsigned n = 0;

        indications[n++] = (MC_TargetIndication){
            .type = MC_WMIND_KEY_DOWN,
            .as.key_down = {.key = get_key(e->wparam)},
        };

        bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        bool paste = (ctrl && e->wparam == 'V') || (shift && e->wparam == VK_INSERT);
        if(paste){
            MC_Str text;
            if(read_clipboard(wm, e->hwnd, &text) == MCE_OK){
                indications[n++] = (MC_TargetIndication){
                    .type = MC_WMIND_PASTE_TEXT,
                    .as.paste_text = {.window = window, .text = text},
                };
            }
        }

        return n;
    }
    case WM_KEYUP:
    case WM_SYSKEYUP:
        indications[0] = (MC_TargetIndication){
            .type = MC_WMIND_KEY_UP,
            .as.key_up = {.key = get_key(e->wparam)},
        };
        return 1;
    case WM_CHAR: {
        wchar_t wc = (wchar_t)e->wparam;
        if(wc < 0x20 || wc == 0x7F){
            return 0;
        }

        char utf8[MC_WM_TEXT_INPUT_CAP];
        int len = WideCharToMultiByte(CP_UTF8, 0, &wc, 1, utf8, sizeof(utf8) - 1, NULL, NULL);
        if(len <= 0){
            return 0;
        }

        indications[0] = (MC_TargetIndication){
            .type = MC_WMIND_TEXT_INPUT,
            .as.text_input = {.window = window},
        };

        memcpy(indications[0].as.text_input.utf8, utf8, (size_t)len);
        indications[0].as.text_input.utf8[len] = '\0';
        return 1;
    }
    default:
        return 0;
    }
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam){
    if(message == WM_NCCREATE){
        CREATESTRUCT *cs = (CREATESTRUCT*)lparam;
        struct MC_TargetWMWindow *window = cs->lpCreateParams;

        window->hwnd = hwnd;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)window);

        return DefWindowProc(hwnd, message, wparam, lparam);
    }

    struct MC_TargetWMWindow *window = window_of(hwnd);
    if(window == NULL){
        return DefWindowProc(hwnd, message, wparam, lparam);
    }

    struct MC_TargetWM *wm = window->wm;

    switch(message){
    case WM_CLOSE:
        enqueue(wm, hwnd, message, wparam, lparam);
        return 0;
    case WM_PAINT:
        enqueue(wm, hwnd, message, wparam, lparam);
        ValidateRect(hwnd, NULL);
        return 0;
    case WM_SIZE:
    case WM_MOVE:
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
    case WM_MOUSEMOVE:
    case WM_MOUSELEAVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP:
    case WM_CHAR:
        enqueue(wm, hwnd, message, wparam, lparam);
        break;
    default:
        break;
    }

    return DefWindowProc(hwnd, message, wparam, lparam);
}

static void enqueue(struct MC_TargetWM *wm, HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam){
    if(wm->queue_count >= EVENT_QUEUE_CAP){
        return;
    }

    unsigned slot = (wm->queue_head + wm->queue_count) % EVENT_QUEUE_CAP;
    wm->queue[slot] = (struct rawevent){
        .hwnd = hwnd,
        .message = message,
        .wparam = wparam,
        .lparam = lparam,
    };

    wm->queue_count++;
}

static MC_MouseButton get_mouse_button(UINT message, WPARAM wparam){
    switch(message){
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
        return MC_MOUSE_LEFT;
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
        return MC_MOUSE_RIGHT;
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
        return MC_MOUSE_MIDDLE;
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP:
        return GET_XBUTTON_WPARAM(wparam) == XBUTTON1 ? MC_MOUSE_BUTTON4 : MC_MOUSE_BUTTON5;
    default:
        return MC_MOUSE_UNKNOWN;
    }
}

static MC_Key get_key(WPARAM vk){
    switch(vk){
    #define MC_VKEY(VK, NAME) case VK: return MC_KEY_##NAME;
    MC_WIN32_ITER_KEYS()
    #undef MC_VKEY
    default:
        return MC_KEY_UNKNOWN;
    }
}

static struct MC_TargetWMWindow *window_of(HWND hwnd){
    return (struct MC_TargetWMWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
}

static MC_Error read_clipboard(struct MC_TargetWM *wm, HWND hwnd, MC_Str *out){
    if(!IsClipboardFormatAvailable(CF_UNICODETEXT)){
        return MCE_NOT_FOUND;
    }

    if(!OpenClipboard(hwnd)){
        return MCE_BUSY;
    }

    HANDLE handle = GetClipboardData(CF_UNICODETEXT);
    if(handle == NULL){
        CloseClipboard();
        return MCE_NOT_FOUND;
    }

    const wchar_t *wide = GlobalLock(handle);
    if(wide == NULL){
        CloseClipboard();
        return MCE_UNKNOWN;
    }

    int len = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
    MC_Error status = MCE_OK;
    if(len <= 0){
        status = MCE_UNKNOWN;
    }
    else{
        char *text;
        status = mc_alloc(wm->arena, (size_t)len, (void**)&text);
        if(status == MCE_OK){
            WideCharToMultiByte(CP_UTF8, 0, wide, -1, text, len, NULL, NULL);
            *out = (MC_Str){.beg = text, .end = text + len - 1};
        }
    }

    GlobalUnlock(handle);
    CloseClipboard();
    return status;
}

static MC_Error request_events(struct MC_TargetWM *wm, MC_WMEvents events){
    if(!(events & (MC_WM_EVENTS_GLOBAL_KEYBOARD | MC_WM_EVENTS_GLOBAL_MOUSE))){
        return MCE_OK;
    }

    if(global_hook_wm != NULL && global_hook_wm != wm){
        LOG("global input is already claimed by another window manager");
        return MCE_BUSY;
    }

    global_hook_wm = wm;

    if((events & MC_WM_EVENTS_GLOBAL_KEYBOARD) && wm->keyboard_hook == NULL){
        wm->keyboard_hook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboard_hook_proc, wm->instance, 0);
        if(wm->keyboard_hook == NULL){
            LOG("SetWindowsHookEx(WH_KEYBOARD_LL) failed (%lu)", (unsigned long)GetLastError());
            return MCE_NOT_SUPPORTED;
        }
    }

    if((events & MC_WM_EVENTS_GLOBAL_MOUSE) && wm->mouse_hook == NULL){
        wm->mouse_hook = SetWindowsHookEx(WH_MOUSE_LL, mouse_hook_proc, wm->instance, 0);
        if(wm->mouse_hook == NULL){
            LOG("SetWindowsHookEx(WH_MOUSE_LL) failed (%lu)", (unsigned long)GetLastError());
            return MCE_NOT_SUPPORTED;
        }
    }

    return MCE_OK;
}

MC_Error mc_wm_win32_identity_from_hwnd(struct MC_TargetWM *wm, HWND hwnd, uint64_t *identity){
    if(hwnd == NULL){
        return MCE_NOT_FOUND;
    }

    struct identity_node *entry;
    MC_Error status = mc_alloc(wm->arena, sizeof(*entry), (void**)&entry);
    if(status != MCE_OK){
        return status;
    }

    entry->next = NULL;
    entry->resolved.id = (uint64_t)(uintptr_t)hwnd;

    if(is_our_window(hwnd)){
        entry->resolved.type = MC_TARGET_IDENTITY_WINDOW;
        entry->resolved.as.window = window_of(hwnd);
    }
    else{
        entry->resolved.type = MC_TARGET_IDENTITY_FOREIGN_WINDOW;
        entry->foreign_window.hwnd = hwnd;
        entry->resolved.as.foreign_window = &entry->foreign_window;
    }

    mc_list_add(&wm->identities, entry);

    *identity = entry->resolved.id;
    return MCE_OK;
}

static MC_Error get_focused_window(struct MC_TargetWM *wm, uint64_t *identity){
    return mc_wm_win32_identity_from_hwnd(wm, GetForegroundWindow(), identity);
}

static MC_Error get_hovered_window(struct MC_TargetWM *wm, uint64_t *identity){
    POINT point;
    if(!GetCursorPos(&point)){
        return MCE_NOT_FOUND;
    }

    HWND hwnd = WindowFromPoint(point);
    if(hwnd != NULL){
        hwnd = GetAncestor(hwnd, GA_ROOT);
    }

    return mc_wm_win32_identity_from_hwnd(wm, hwnd, identity);
}

struct enum_collect{
    struct MC_TargetWM *wm;
    uint64_t *ids;
    size_t count;
    MC_Error status;
};

static BOOL CALLBACK enum_windows_proc(HWND hwnd, LPARAM lparam){
    struct enum_collect *collect = (struct enum_collect*)lparam;

    if(!IsWindowVisible(hwnd) || GetWindowTextLengthW(hwnd) == 0){
        return TRUE;
    }

    if(collect->count >= ENUM_WINDOWS_CAP){
        return FALSE;
    }

    uint64_t identity;
    MC_Error status = mc_wm_win32_identity_from_hwnd(collect->wm, hwnd, &identity);
    if(status != MCE_OK){
        collect->status = status;
        return FALSE;
    }

    collect->ids[collect->count++] = identity;
    return TRUE;
}

static MC_Error get_all_windows(struct MC_TargetWM *wm, uint64_t **identities, size_t *count){
    uint64_t *ids;
    MC_Error status = mc_alloc(wm->arena, ENUM_WINDOWS_CAP * sizeof(*ids), (void**)&ids);
    if(status != MCE_OK){
        return status;
    }

    struct enum_collect collect = {.wm = wm, .ids = ids, .count = 0, .status = MCE_OK};
    EnumWindows(enum_windows_proc, (LPARAM)&collect);

    *identities = ids;
    *count = collect.count;
    return collect.status;
}

static bool is_our_window(HWND hwnd){
    char cls[64];
    int n = GetClassNameA(hwnd, cls, (int)sizeof(cls));
    return n > 0 && strcmp(cls, WINDOW_CLASS_NAME) == 0;
}

static MC_Error resolve_temporary_identity(struct MC_TargetWM *wm, uint64_t identity, MC_TargetResolvedIdentity *out){
    MC_LIST_FOR(struct identity_node, wm->identities, entry){
        if(entry->resolved.id == identity){
            *out = entry->resolved;
            return MCE_OK;
        }
    }

    return MCE_NOT_FOUND;
}

static void heartbeat(struct MC_TargetWM *wm){
    wm->identities = NULL;
}

static MC_Error set_foreign_window_title(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window, MC_Str title){
    (void)wm;
    return hwnd_set_title(window->hwnd, title);
}

static MC_Error set_foreign_window_rect(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window, MC_WMArea area, MC_Rect2IU rect){
    (void)wm;
    return hwnd_set_rect(window->hwnd, area, rect);
}

static MC_Error set_foreign_window_state(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window, MC_WMWindowState state){
    (void)wm;
    return hwnd_set_state(window->hwnd, state);
}

static MC_Error focus_foreign_window(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window){
    (void)wm;
    return hwnd_focus(window->hwnd);
}

static MC_Error get_foreign_window_title(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window, char *utf8, size_t cap, size_t *len){
    (void)wm;
    return hwnd_get_title(window->hwnd, utf8, cap, len);
}

static MC_Error get_foreign_window_state(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window, MC_WMWindowState *state){
    (void)wm;
    return hwnd_get_state(window->hwnd, state);
}

static MC_Error is_foreign_window_system(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window, bool *out){
    (void)wm;
    return hwnd_is_system(window->hwnd, out);
}

static MC_Error get_foreign_window_rect(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window, MC_WMArea area, MC_Rect2IU *rect){
    (void)wm;
    return hwnd_get_rect(window->hwnd, area, rect);
}

static unsigned translate_global_event(const struct rawevent *e, MC_TargetIndication indications[MC_WM_MAX_INDICATIONS_PER_EVENT]){
    MC_Vec2i position = unpack_point(e->lparam);

    switch(e->message){
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        indications[0] = (MC_TargetIndication){
            .type = MC_WMIND_GLOBAL_KEY_DOWN,
            .as.global_key_down = {.key = get_key(e->wparam)},
        };
        return 1;
    case WM_KEYUP:
    case WM_SYSKEYUP:
        indications[0] = (MC_TargetIndication){
            .type = MC_WMIND_GLOBAL_KEY_UP,
            .as.global_key_up = {.key = get_key(e->wparam)},
        };
        return 1;
    case WM_MOUSEMOVE:
        indications[0] = (MC_TargetIndication){
            .type = MC_WMIND_GLOBAL_MOUSE_MOVED,
            .as.global_mouse_moved = {.position = position},
        };
        return 1;
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_XBUTTONDOWN:
        indications[0] = (MC_TargetIndication){
            .type = MC_WMIND_GLOBAL_MOUSE_DOWN,
            .as.global_mouse_down = {.position = position, .button = get_mouse_button(e->message, e->wparam)},
        };
        return 1;
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_XBUTTONUP:
        indications[0] = (MC_TargetIndication){
            .type = MC_WMIND_GLOBAL_MOUSE_UP,
            .as.global_mouse_up = {.position = position, .button = get_mouse_button(e->message, e->wparam)},
        };
        return 1;
    case WM_MOUSEWHEEL:
        indications[0] = (MC_TargetIndication){
            .type = MC_WMIND_GLOBAL_MOUSE_WHEEL,
            .as.global_mouse_wheel = {.position = position, .up = GET_WHEEL_DELTA_WPARAM(e->wparam) / WHEEL_DELTA, .right = 0},
        };
        return 1;
    case WM_MOUSEHWHEEL:
        indications[0] = (MC_TargetIndication){
            .type = MC_WMIND_GLOBAL_MOUSE_WHEEL,
            .as.global_mouse_wheel = {.position = position, .up = 0, .right = GET_WHEEL_DELTA_WPARAM(e->wparam) / WHEEL_DELTA},
        };
        return 1;
    default:
        return 0;
    }
}

static void enqueue_global(struct MC_TargetWM *wm, UINT message, WPARAM wparam, POINT point){
    if(wm->queue_count >= EVENT_QUEUE_CAP){
        return;
    }

    unsigned slot = (wm->queue_head + wm->queue_count) % EVENT_QUEUE_CAP;
    wm->queue[slot] = (struct rawevent){
        .hwnd = NULL,
        .message = message,
        .wparam = wparam,
        .lparam = pack_point(point),
    };

    wm->queue_count++;
}

static LPARAM pack_point(POINT point){
    return ((LPARAM)(DWORD)point.x) | ((LPARAM)(DWORD)point.y << 32);
}

static MC_Vec2i unpack_point(LPARAM lparam){
    return (MC_Vec2i){
        .x = (int)(LONG)(DWORD)lparam,
        .y = (int)(LONG)(DWORD)(lparam >> 32),
    };
}

static LRESULT CALLBACK keyboard_hook_proc(int code, WPARAM wparam, LPARAM lparam){
    if(code == HC_ACTION && global_hook_wm){
        KBDLLHOOKSTRUCT *kb = (KBDLLHOOKSTRUCT*)lparam;
        enqueue_global(global_hook_wm, (UINT)wparam, kb->vkCode, (POINT){0, 0});

        if(global_hook_wm->keyboard_suppress){
            bool down = wparam == WM_KEYDOWN || wparam == WM_SYSKEYDOWN;
            if(global_hook_wm->keyboard_suppress(global_hook_wm, get_key(kb->vkCode), down)){
                return 1;
            }
        }
    }

    return CallNextHookEx(NULL, code, wparam, lparam);
}

static LRESULT CALLBACK mouse_hook_proc(int code, WPARAM wparam, LPARAM lparam){
    if(code == HC_ACTION && global_hook_wm){
        MSLLHOOKSTRUCT *ms = (MSLLHOOKSTRUCT*)lparam;
        enqueue_global(global_hook_wm, (UINT)wparam, ms->mouseData, ms->pt);

        if(global_hook_wm->mouse_suppress){
            if(global_hook_wm->mouse_suppress(global_hook_wm, (UINT)wparam, ms->pt.x, ms->pt.y)){
                return 1;
            }
        }
    }

    return CallNextHookEx(NULL, code, wparam, lparam);
}
