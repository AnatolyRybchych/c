#ifndef MC_WM_TARGET_H
#define MC_WM_TARGET_H

#include <mc/wm/wm.h>
#include <mc/wm/mouse_button.h>

#define MC_WM_MAX_INDICATIONS_PER_EVENT 16

#define MC_ITER_INDICATIONS() \
    MC_WMIDN(WINDOW_READY,              UNIQUE_PER_WINDOW) \
    MC_WMIDN(WINDOW_RESIZED,            USE_LAST) \
    MC_WMIDN(WINDOW_MOVED,              USE_LAST) \
    MC_WMIDN(WINDOW_HIDE,               USE_ALL) \
    MC_WMIDN(WINDOW_SHOW,               USE_ALL) \
    MC_WMIDN(WINDOW_REDRAW_REQUESTED,   USE_LAST) \
    MC_WMIDN(MOUSE_MOVED,               USE_ALL) \
    MC_WMIDN(MOUSE_DOWN,                USE_ALL) \
    MC_WMIDN(MOUSE_UP,                  USE_ALL) \
    MC_WMIDN(MOUSE_ENTER,               USE_ALL) \
    MC_WMIDN(MOUSE_LEAVE,               USE_ALL) \
    MC_WMIDN(MOUSE_WHEEL,               USE_ALL) \

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
struct MC_TargetWMEvent;

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
            MC_Point2I new_position;
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

        struct MC_WMIND_MouseMoved{
            struct MC_TargetWMWindow *window;
            MC_Point2I position;
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
    } as;
};

struct MC_WMVtab{
    char name[32];

    size_t wm_size;
    size_t window_size;
    size_t event_size;

    MC_Error (*init)(struct MC_TargetWM *wm, MC_Stream *log);
    void (*destroy)(struct MC_TargetWM *wm);

    MC_Error (*init_window)(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window);
    void (*destroy_window)(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window);

    MC_Error (*create_window_graphic)(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, struct MC_Graphics **g);

    MC_Error (*set_window_title)(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_Str title);
    MC_Error (*set_window_position)(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_Point2I position);
    MC_Error (*set_window_size)(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_Size2U size);
    MC_Error (*set_window_rect)(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_Rect2IU rect);

    MC_Error (*get_window_title)(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_Stream *title);
    MC_Error (*get_window_position)(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_Point2I *position);
    MC_Error (*get_window_size)(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_Size2U *size);
    MC_Error (*get_window_rect)(struct MC_TargetWM *wm, struct MC_TargetWMWindow *window, MC_Rect2IU *rect);

    bool (*poll_event)(struct MC_TargetWM *wm, struct MC_TargetWMEvent *event);
    unsigned (*translate_event)(struct MC_TargetWM *wm, const struct MC_TargetWMEvent *event, MC_TargetIndication indications[MC_WM_MAX_INDICATIONS_PER_EVENT]);
};

#endif // MC_WM_TARGET_H
