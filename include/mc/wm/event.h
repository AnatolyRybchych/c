#ifndef MC_WM_EVENT_H
#define MC_WM_EVENT_H

#include <mc/wm/wm.h>
#include <mc/wm/mouse_button.h>

#define MC_ITER_WM_EVENTS() \
    MC_EVENT(NONE) \
    MC_EVENT(RAW) \
    MC_EVENT(WINDOW_READY) \
    MC_EVENT(WINDOW_RESIZED) \
    MC_EVENT(WINDOW_MOVED) \
    MC_EVENT(WINDOW_REDRAW_REQUESTED) \
    MC_EVENT(MOUSE_MOVED) \
    MC_EVENT(MOUSE_DOWN) \
    MC_EVENT(MOUSE_UP) \
    MC_EVENT(MOUSE_CLICK) \
    MC_EVENT(MOUSE_ENTER) \
    MC_EVENT(MOUSE_LEAVE) \
    MC_EVENT(MOUSE_WHEEL) \

typedef unsigned MC_WMEventType;
enum MC_WMEventType{
#define MC_EVENT(NAME, ...) MC_WME_##NAME,
    MC_ITER_WM_EVENTS()
#undef MC_EVENT

    MC_WME_COUNT
};

inline const char *mc_wm_event_type_str(MC_WMEventType type){
    switch (type){
    #define MC_EVENT(NAME, ...) case MC_WME_##NAME: return #NAME;
        MC_ITER_WM_EVENTS()
    #undef MC_EVENT
    default: return NULL;
    }
}

struct MC_WMEvent{
    MC_WMEventType type;
    union {
        void *raw;

        struct MC_WME_WindowReady{
            MC_WMWindow *window;
        } window_ready;

        struct MC_WME_WindowResized{
            MC_WMWindow *window;
            MC_Size2U new_size;
        } window_resized;

        struct MC_WME_WindowMoved{
            MC_WMWindow *window;
            MC_Point2I new_position;
        } window_moved;

        struct MC_WME_RedrawRequest{
            MC_WMWindow *window;
        } redraw_requested;

        struct MC_WME_MouseMoved{
            struct MC_WMWindow *window;
            MC_Point2I position;
        } mouse_moved;

        struct MC_WME_MouseDown{
            struct MC_WMWindow *window;
            MC_Point2I position;
            MC_MouseButton button;
        } mouse_down;

        struct MC_WME_MouseUp{
            struct MC_WMWindow *window;
            MC_Point2I position;
            MC_MouseButton button;
        } mouse_up;

        struct MC_WME_MouseEnter{
            struct MC_WMWindow *window;
            MC_Point2I position;
        } mouse_enter;

        struct MC_WME_MouseLeave{
            struct MC_WMWindow *window;
            MC_Point2I position;
        } mouse_leave;

        struct MC_WME_MouseWheel{
            struct MC_WMWindow *window;
            MC_Point2I position;
            int up;
            int right;
        } mouse_wheel;
    } as;
};

#endif // MC_WM_EVENT_H
