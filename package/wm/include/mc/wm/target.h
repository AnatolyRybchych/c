#ifndef MC_WM_TARGET_H
#define MC_WM_TARGET_H

#include <mc/wm/wm.h>
#include <mc/wm/mouse_button.h>
#include <mc/wm/key.h>

#include <mc/data/alloc.h>

#include <stdint.h>

#define MC_WM_MAX_INDICATIONS_PER_EVENT 16

#define MC_ITER_INDICATIONS() \
    MC_WMIDN(WINDOW_READY,            UNIQUE_PER_WINDOW, WINDOW_CORE) \
    MC_WMIDN(WINDOW_RESIZED,          USE_LAST,          WINDOW_CORE) \
    MC_WMIDN(WINDOW_MOVED,            USE_LAST,          WINDOW_CORE) \
    MC_WMIDN(WINDOW_HIDE,             USE_ALL,           WINDOW_CORE) \
    MC_WMIDN(WINDOW_SHOW,             USE_ALL,           WINDOW_CORE) \
    MC_WMIDN(WINDOW_REDRAW_REQUESTED, USE_LAST,          WINDOW_CORE) \
    MC_WMIDN(WINDOW_CLOSE_REQUESTED,  USE_LAST,          WINDOW_CORE) \
    MC_WMIDN(WINDOW_STATE_CHANGED,    USE_LAST,          WINDOW_CORE) \
    MC_WMIDN(FOCUS_GAINED,            USE_LAST,          WINDOW_CORE) \
    MC_WMIDN(FOCUS_LOST,              USE_LAST,          WINDOW_CORE) \
    MC_WMIDN(MOUSE_MOVED,             USE_ALL,           WINDOW_CORE) \
    MC_WMIDN(MOUSE_DOWN,              USE_ALL,           WINDOW_CORE) \
    MC_WMIDN(MOUSE_UP,                USE_ALL,           WINDOW_CORE) \
    MC_WMIDN(MOUSE_ENTER,             USE_ALL,           WINDOW_CORE) \
    MC_WMIDN(MOUSE_LEAVE,             USE_ALL,           WINDOW_CORE) \
    MC_WMIDN(MOUSE_WHEEL,             USE_ALL,           WINDOW_CORE) \
    MC_WMIDN(KEY_DOWN,                USE_ALL,           WINDOW_CORE) \
    MC_WMIDN(KEY_UP,                  USE_ALL,           WINDOW_CORE) \
    MC_WMIDN(TEXT_INPUT,              USE_ALL,           WINDOW_CORE) \
    MC_WMIDN(PASTE_TEXT,              USE_ALL,           WINDOW_CORE) \
    MC_WMIDN(GLOBAL_KEY_DOWN,         USE_ALL,           GLOBAL_KEYBOARD) \
    MC_WMIDN(GLOBAL_KEY_UP,           USE_ALL,           GLOBAL_KEYBOARD) \
    MC_WMIDN(GLOBAL_MOUSE_MOVED,      USE_ALL,           GLOBAL_MOUSE) \
    MC_WMIDN(GLOBAL_MOUSE_DOWN,       USE_ALL,           GLOBAL_MOUSE) \
    MC_WMIDN(GLOBAL_MOUSE_UP,         USE_ALL,           GLOBAL_MOUSE) \
    MC_WMIDN(GLOBAL_MOUSE_WHEEL,      USE_ALL,           GLOBAL_MOUSE) \

typedef unsigned MC_WMIndicationType;
enum MC_WMIndicationType{
#define MC_WMIDN(NAME, ...) MC_WMIND_##NAME,
    MC_ITER_INDICATIONS()
#undef MC_WMIDN

    MC_WMIND_COUNT
};

typedef unsigned MC_TargetIndicationDuplicateAction;
enum MC_TargetIndicationDuplicateAction{
    MC_WM_INDDUP_USE_ALL,
    MC_WM_INDDUP_USE_LAST,
    MC_WM_INDDUP_UNIQUE_PER_WINDOW,
};

typedef struct MC_TargetIndication MC_TargetIndication;

inline const char *mc_wm_target_indication_type_str(MC_WMIndicationType ind){
    switch (ind){
    #define MC_WMIDN(NAME, ...) case MC_WMIND_##NAME: return #NAME;
        MC_ITER_INDICATIONS()
    #undef MC_WMIDN
    default: return NULL;
    }
}

struct MC_TargetWM;
struct MC_TargetWMWindow;
struct MC_TargetForeignWindow;
struct MC_TargetWMEvent;

typedef unsigned MC_TargetIdentityType;
enum MC_TargetIdentityType{
    MC_TARGET_IDENTITY_FOREIGN_WINDOW,
    MC_TARGET_IDENTITY_WINDOW,
};

typedef struct MC_TargetResolvedIdentity{
    MC_TargetIdentityType type;
    uint64_t id;
    union{
        struct MC_TargetForeignWindow *foreign_window;
        struct MC_TargetWMWindow *window;
    } as;
} MC_TargetResolvedIdentity;

struct MC_TargetIndication{
    MC_WMIndicationType type;
    union {
        struct MC_WMIND_WindowReady{
            struct MC_TargetWMWindow *window;
        } window_ready;

        struct MC_WMIND_WindowResized{
            struct MC_TargetWMWindow *window;
            MC_Size2U new_size;
        } window_resized;

        struct MC_WMIND_WindowMoved{
            struct MC_TargetWMWindow *window;
            MC_Vec2i new_position;
        } window_moved;

        struct MC_WMIND_WindowHide{
            struct MC_TargetWMWindow *window;
        } window_hide;

        struct MC_WMIND_WindowShow{
            struct MC_TargetWMWindow *window;
        } window_show;

        struct MC_WMIND_RedrawRequested{
            struct MC_TargetWMWindow *window;
        } redraw_requested;

        struct MC_WMIND_WindowStateChanged{
            struct MC_TargetWMWindow *window;
            MC_WMWindowState state;
        } window_state_changed;

        struct MC_WMIND_WindowCloseRequested{
            struct MC_TargetWMWindow *window;
        } window_close_requested;

        struct MC_WMIND_FocusGained{
            struct MC_TargetWMWindow *window;
        } focus_gained;

        struct MC_WMIND_FocusLost{
            struct MC_TargetWMWindow *window;
        } focus_lost;

        struct MC_WMIND_MouseMoved{
            struct MC_TargetWMWindow *window;
            MC_Vec2i position;
        } mouse_moved;

        struct MC_WMIND_MouseDown{
            struct MC_TargetWMWindow *window;
            MC_MouseButton button;
        } mouse_down;

        struct MC_WMIND_MouseUp{
            struct MC_TargetWMWindow *window;
            MC_MouseButton button;
        } mouse_up;

        struct MC_WMIND_MouseEnter{
            struct MC_TargetWMWindow *window;
        } mouse_enter;

        struct MC_WMIND_MouseLeave{
            struct MC_TargetWMWindow *window;
        } mouse_leave;

        struct MC_WMIND_MouseWheel{
            struct MC_TargetWMWindow *window;
            int up;
            int right;
        } mouse_wheel;

        struct MC_WMIND_KeyDown{
            MC_Key key;
        } key_down;

        struct MC_WMIND_KeyUp{
            MC_Key key;
        } key_up;

        struct MC_WMIND_TextInput{
            struct MC_TargetWMWindow *window;
            char utf8[MC_WM_TEXT_INPUT_CAP];
        } text_input;

        struct MC_WMIND_PasteText{
            struct MC_TargetWMWindow *window;
            MC_Str text;
        } paste_text;

        struct MC_WMIND_GlobalKeyDown{
            MC_Key key;
        } global_key_down;

        struct MC_WMIND_GlobalKeyUp{
            MC_Key key;
        } global_key_up;

        struct MC_WMIND_GlobalMouseMoved{
            MC_Vec2i position;
        } global_mouse_moved;

        struct MC_WMIND_GlobalMouseDown{
            MC_Vec2i position;
            MC_MouseButton button;
        } global_mouse_down;

        struct MC_WMIND_GlobalMouseUp{
            MC_Vec2i position;
            MC_MouseButton button;
        } global_mouse_up;

        struct MC_WMIND_GlobalMouseWheel{
            MC_Vec2i position;
            int up;
            int right;
        } global_mouse_wheel;
    } as;
};

struct MC_WMVtab{
    char name[32];

    size_t wm_size;
    size_t window_size;
    size_t foreign_window_size;
    size_t event_size;

    MC_Error (*init)(struct MC_TargetWM *wm, MC_Stream *log, MC_Alloc *arena);
    void (*destroy)(struct MC_TargetWM *wm);

    MC_Error (*init_window)(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window);
    void (*destroy_window)(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window);

    MC_Error (*create_window_graphic)(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, struct MC_Graphics **g);

    MC_Error (*set_window_title)(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_Str title);
    MC_Error (*set_window_position)(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_Vec2i position);
    MC_Error (*set_window_size)(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_Size2U size);
    MC_Error (*set_window_rect)(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_Rect2IU rect);
    MC_Error (*set_window_state)(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_WMWindowState state);

    MC_Error (*get_window_title)(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_Stream *title);
    MC_Error (*get_window_position)(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_Vec2i *position);
    MC_Error (*get_window_size)(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_Size2U *size);
    MC_Error (*get_window_rect)(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_Rect2IU *rect);

    bool (*poll_event)(struct MC_TargetWM *wm, struct MC_TargetWMEvent *event);
    unsigned (*translate_event)(struct MC_TargetWM *wm, const struct MC_TargetWMEvent *event, MC_TargetIndication indications[MC_WM_MAX_INDICATIONS_PER_EVENT]);

    MC_Error (*request_events)(struct MC_TargetWM *wm, MC_WMEvents events);

    MC_Error (*get_focused_window)(struct MC_TargetWM *wm, uint64_t *identity);
    MC_Error (*resolve_temporary_identity)(struct MC_TargetWM *wm, uint64_t identity, MC_TargetResolvedIdentity *out);
    void (*heartbeat)(struct MC_TargetWM *wm);
    void (*destroy_foreign_window)(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window);
    MC_Error (*close_foreign_window)(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window);

    MC_Error (*set_foreign_window_title)(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window, MC_Str title);
    MC_Error (*set_foreign_window_rect)(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window, MC_Rect2IU rect);
    MC_Error (*set_foreign_window_state)(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window, MC_WMWindowState state);

    MC_Error (*get_foreign_window_title)(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window, char *utf8, size_t cap, size_t *len);
    MC_Error (*get_foreign_window_rect)(struct MC_TargetWM *wm, struct MC_TargetForeignWindow *window, MC_Rect2IU *rect);
};

#endif // MC_WM_TARGET_H
