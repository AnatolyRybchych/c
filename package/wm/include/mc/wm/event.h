#ifndef MC_WM_EVENT_H
#define MC_WM_EVENT_H

#include <mc/wm/wm.h>
#include <mc/wm/mouse_button.h>
#include <mc/wm/key.h>
#include <mc/data/json.h>

#include <string.h>
#include <assert.h>

#define MC_ITER_WM_EVENTS() \
    MC_EVENT(NONE,                      NONE) \
    MC_EVENT(RAW,                       GENERIC.RAW) \
    MC_EVENT(WINDOW_READY,              WINDOW.READY) \
    MC_EVENT(WINDOW_RESIZED,            WINDOW.RESIZED) \
    MC_EVENT(WINDOW_MOVED,              WINDOW.MOVED) \
    MC_EVENT(WINDOW_REDRAW_REQUESTED,   WINDOW.REDRAW_REQUESTED) \
    MC_EVENT(WINDOW_CLOSE_REQUESTED,    WINDOW.CLOSE_REQUESTED) \
    MC_EVENT(WINDOW_STATE_CHANGED,      WINDOW.STATE_CHANGED) \
    MC_EVENT(FOCUS_GAINED,              WINDOW.FOCUS_GAINED) \
    MC_EVENT(FOCUS_LOST,                WINDOW.FOCUS_LOST) \
    MC_EVENT(MOUSE_MOVED,               WINDOW.MOUSE_MOVED) \
    MC_EVENT(MOUSE_DOWN,                WINDOW.MOUSE_DOWN) \
    MC_EVENT(MOUSE_UP,                  WINDOW.MOUSE_UP) \
    MC_EVENT(MOUSE_CLICK,               WINDOW.MOUSE_CLICK) \
    MC_EVENT(MOUSE_ENTER,               WINDOW.MOUSE_ENTER) \
    MC_EVENT(MOUSE_LEAVE,               WINDOW.MOUSE_LEAVE) \
    MC_EVENT(MOUSE_WHEEL,               WINDOW.MOUSE_WHEEL) \
    MC_EVENT(KEY_DOWN,                  WINDOW.KEY_DOWN) \
    MC_EVENT(KEY_UP,                    WINDOW.KEY_UP) \
    MC_EVENT(TEXT_INPUT,                WINDOW.TEXT_INPUT) \
    MC_EVENT(PASTE_TEXT,                WINDOW.PASTE_TEXT) \
    MC_EVENT(GLOBAL_KEY_DOWN,           GLOBAL.KEY_DOWN) \
    MC_EVENT(GLOBAL_KEY_UP,             GLOBAL.KEY_UP) \
    MC_EVENT(GLOBAL_MOUSE_MOVED,        GLOBAL.MOUSE_MOVED) \
    MC_EVENT(GLOBAL_MOUSE_DOWN,         GLOBAL.MOUSE_DOWN) \
    MC_EVENT(GLOBAL_MOUSE_UP,           GLOBAL.MOUSE_UP) \
    MC_EVENT(GLOBAL_MOUSE_WHEEL,        GLOBAL.MOUSE_WHEEL) \

typedef uint16_t MC_WMEventType;

enum MC_WMEventType{
#define MC_EVENT(NAME, SERIAL) MC_WME_##NAME,
    MC_ITER_WM_EVENTS()
#undef MC_EVENT
    MC_WME_WM_COUNT,
};

inline const char *mc_wm_wm_event_name(MC_WMEventType type){
    switch (type){
    #define MC_EVENT(NAME, SERIAL) case MC_WME_##NAME: return #SERIAL;
        MC_ITER_WM_EVENTS()
    #undef MC_EVENT
    default:
        return NULL;
    }
}

typedef struct MC_WMEventDefinition{
    const char *name;
} MC_WMEventDefinition;

typedef struct MC_WMEventGroupDef{
    const char *name;
    const MC_WMEventDefinition *events;
    size_t size;
    size_t reserve;
    MC_Error (*to_json)(MC_Alloc *alloc, const MC_WMEvent *event, MC_Json *json);
    MC_Error (*from_json)(MC_Alloc *alloc, const MC_Json *json, MC_WMEvent *event);
} MC_WMEventGroupDef;

MC_Alloc *mc_wm_event_allocator(MC_WMRef *wm);

MC_Error mc_wm_register_event_group(MC_WMRef *wm, const MC_WMEventGroupDef *def, MC_WMEventType *out_offset);
MC_WMEventType mc_wm_event_group_offset(MC_WMRef *wm, const char *group);
MC_WMEventType mc_wm_event_type_from_str(MC_WMRef *wm, const char *name);
const char *mc_wm_event_type_str(MC_WMRef *wm, MC_WMEventType type);

MC_Error mc_wm_event(MC_WMRef *wm, MC_WMEventType type, MC_WMEvent *out);
MC_Error mc_wm_push_event(MC_WMRef *wm, const MC_WMEvent *event);
void mc_wm_set_event_queue_limit(MC_WMRef *wm, size_t limit);

struct MC_WMEvent{
    MC_WMEventType type;
    union {
        void *raw;

        uint64_t window;

        struct MC_WME_WindowReady{
            uint64_t window;
        } window_ready;

        struct MC_WME_WindowResized{
            uint64_t window;
            MC_Size2U new_size;
        } window_resized;

        struct MC_WME_WindowMoved{
            uint64_t window;
            MC_Vec2i new_position;
        } window_moved;

        struct MC_WME_RedrawRequest{
            uint64_t window;
        } redraw_requested;

        struct MC_WME_WindowStateChanged{
            uint64_t window;
            MC_WMWindowState state;
        } window_state_changed;

        struct MC_WME_WindowCloseRequest{
            uint64_t window;
        } window_close_requested;

        struct MC_WME_FocusGained{
            uint64_t window;
        } focus_gained;

        struct MC_WME_FocusLost{
            uint64_t window;
        } focus_lost;

        struct MC_WME_MouseMoved{
            uint64_t window;
            MC_Vec2i position;
        } mouse_moved;

        struct MC_WME_MouseDown{
            uint64_t window;
            MC_Vec2i position;
            MC_MouseButton button;
        } mouse_down;

        struct MC_WME_MouseUp{
            uint64_t window;
            MC_Vec2i position;
            MC_MouseButton button;
        } mouse_up;

        struct MC_WME_MouseEnter{
            uint64_t window;
            MC_Vec2i position;
        } mouse_enter;

        struct MC_WME_MouseLeave{
            uint64_t window;
            MC_Vec2i position;
        } mouse_leave;

        struct MC_WME_MouseWheel{
            uint64_t window;
            MC_Vec2i position;
            int up;
            int right;
        } mouse_wheel;

        struct MC_WME_KeyDown{
            uint64_t window;
            MC_Key key;
        } key_down;

        struct MC_WME_KeyUp{
            uint64_t window;
            MC_Key key;
        } key_up;

        struct MC_WME_TextInput{
            uint64_t window;
            char utf8[MC_WM_TEXT_INPUT_CAP];
        } text_input;

        struct MC_WME_PasteText{
            uint64_t window;
            MC_Str text;
        } paste_text;

        struct MC_WME_GlobalKeyDown{
            MC_Key key;
        } global_key_down;

        struct MC_WME_GlobalKeyUp{
            MC_Key key;
        } global_key_up;

        struct MC_WME_GlobalMouseMoved{
            MC_Vec2i position;
        } global_mouse_moved;

        struct MC_WME_GlobalMouseDown{
            MC_Vec2i position;
            MC_MouseButton button;
        } global_mouse_down;

        struct MC_WME_GlobalMouseUp{
            MC_Vec2i position;
            MC_MouseButton button;
        } global_mouse_up;

        struct MC_WME_GlobalMouseWheel{
            MC_Vec2i position;
            int up;
            int right;
        } global_mouse_wheel;
    } as;
};

typedef struct MC_WMEventSubscription MC_WMEventSubscription;

typedef struct MC_WMEventMatch{
    MC_WMEventType type;
} MC_WMEventMatch;

MC_Error mc_wm_subscribe_event(MC_WMRef *wm, MC_WMEventMatch match,
    void (*callback)(MC_WMRef *wm, const MC_WMEvent *event, void *user_data),
    void *user_data, MC_WMEventSubscription **out);

void mc_wm_unsubscribe_event(MC_WMEventSubscription *subscription);
void mc_wm_dispatch_event_callbacks(MC_WMRef *wm, const MC_WMEvent *event);

MC_Error mc_wm_event_to_json(MC_WMRef *wm, const MC_WMEvent *event, MC_Json *out);
MC_Error mc_wm_event_from_json(MC_WMRef *wm, MC_Alloc *alloc, const MC_Json *json, MC_WMEvent *out);

#endif // MC_WM_EVENT_H
