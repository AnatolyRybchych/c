#ifndef MC_WM_EVENT_H
#define MC_WM_EVENT_H

#include <mc/wm/wm.h>

#define MC_ITER_WM_EVENTS() \
    MC_EVENT(NONE) \
    MC_EVENT(RAW) \
    MC_EVENT(WINDOW_READY) \
    MC_EVENT(WINDOW_RESIZED) \
    MC_EVENT(WINDOW_MOVED) \
    MC_EVENT(MOUSE_MOVED) \
    MC_EVENT(MOUSE_DOWN) \
    MC_EVENT(MOUSE_UP) \
    MC_EVENT(MOUSE_CLICK) \

typedef unsigned MC_WMEventType;
enum MC_WMEventType{
#define MC_EVENT(NAME, ...) MC_WME_##NAME,
    MC_ITER_WM_EVENTS()
#undef MC_EVENT

    MC_WME_COUNT
};

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
        } redraw_request;
    } as;
};

#endif // MC_WM_EVENT_H
