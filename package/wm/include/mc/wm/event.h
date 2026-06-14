#ifndef MC_WM_EVENT_H
#define MC_WM_EVENT_H

#include <mc/wm/wm.h>
#include <mc/wm/mouse_button.h>
#include <mc/wm/key.h>
#include <mc/data/json.h>

#include <string.h>
#include <assert.h>

#define MC_ITER_WM_EVENTS() \
    MC_EVENT(NONE,                      NONE,    NONE) \
    MC_EVENT(RAW,                       GENERIC, GENERIC.RAW) \
    MC_EVENT(WINDOW_READY,              WINDOW,  WINDOW.READY) \
    MC_EVENT(WINDOW_RESIZED,            WINDOW,  WINDOW.RESIZED) \
    MC_EVENT(WINDOW_MOVED,              WINDOW,  WINDOW.MOVED) \
    MC_EVENT(WINDOW_REDRAW_REQUESTED,   WINDOW,  WINDOW.REDRAW_REQUESTED) \
    MC_EVENT(WINDOW_CLOSE_REQUESTED,    WINDOW,  WINDOW.CLOSE_REQUESTED) \
    MC_EVENT(WINDOW_STATE_CHANGED,      WINDOW,  WINDOW.STATE_CHANGED) \
    MC_EVENT(FOCUS_GAINED,              WINDOW,  WINDOW.FOCUS_GAINED) \
    MC_EVENT(FOCUS_LOST,                WINDOW,  WINDOW.FOCUS_LOST) \
    MC_EVENT(MOUSE_MOVED,               WINDOW,  WINDOW.MOUSE_MOVED) \
    MC_EVENT(MOUSE_DOWN,                WINDOW,  WINDOW.MOUSE_DOWN) \
    MC_EVENT(MOUSE_UP,                  WINDOW,  WINDOW.MOUSE_UP) \
    MC_EVENT(MOUSE_CLICK,               WINDOW,  WINDOW.MOUSE_CLICK) \
    MC_EVENT(MOUSE_ENTER,               WINDOW,  WINDOW.MOUSE_ENTER) \
    MC_EVENT(MOUSE_LEAVE,               WINDOW,  WINDOW.MOUSE_LEAVE) \
    MC_EVENT(MOUSE_WHEEL,               WINDOW,  WINDOW.MOUSE_WHEEL) \
    MC_EVENT(KEY_DOWN,                  WINDOW,  WINDOW.KEY_DOWN) \
    MC_EVENT(KEY_UP,                    WINDOW,  WINDOW.KEY_UP) \
    MC_EVENT(TEXT_INPUT,                WINDOW,  WINDOW.TEXT_INPUT) \
    MC_EVENT(PASTE_TEXT,                WINDOW,  WINDOW.PASTE_TEXT) \
    MC_EVENT(GLOBAL_KEY_DOWN,           GLOBAL,  GLOBAL.KEY_DOWN) \
    MC_EVENT(GLOBAL_KEY_UP,             GLOBAL,  GLOBAL.KEY_UP) \
    MC_EVENT(GLOBAL_MOUSE_MOVED,        GLOBAL,  GLOBAL.MOUSE_MOVED) \
    MC_EVENT(GLOBAL_MOUSE_DOWN,         GLOBAL,  GLOBAL.MOUSE_DOWN) \
    MC_EVENT(GLOBAL_MOUSE_UP,           GLOBAL,  GLOBAL.MOUSE_UP) \
    MC_EVENT(GLOBAL_MOUSE_WHEEL,        GLOBAL,  GLOBAL.MOUSE_WHEEL) \

typedef uint16_t MC_WMEventGroup;
enum MC_WMEventGroup{
    MC_WME_GROUP_NONE    = 0,
    MC_WME_GROUP_GENERIC = 1,
    MC_WME_GROUP_WINDOW  = 2,
    MC_WME_GROUP_GLOBAL  = 3,
    MC_WME_GROUP_APP     = 4,
    MC_WME_GROUP_USER    = 5,
};

#define MC_WME_GROUP_BITS 4
#define MC_WME_GROUP_SHIFT (16 - MC_WME_GROUP_BITS)

static_assert(MC_WME_GROUP_USER < (1 << MC_WME_GROUP_BITS), "MC_WMEventGroup must fit in MC_WME_GROUP_BITS bits");

typedef uint16_t MC_WMEventType;

enum {
#define MC_EVENT(NAME, GROUP, SERIAL) MC_WME_INDEX_##NAME,
    MC_ITER_WM_EVENTS()
#undef MC_EVENT
};

enum MC_WMEventType{
#define MC_EVENT(NAME, GROUP, SERIAL) MC_WME_##NAME = MC_WME_INDEX_##NAME | (MC_WME_GROUP_##GROUP << MC_WME_GROUP_SHIFT),
    MC_ITER_WM_EVENTS()
#undef MC_EVENT
};

inline MC_WMEventGroup mc_wm_event_type_group(MC_WMEventType type){
    return type >> MC_WME_GROUP_SHIFT;
}

inline const char *mc_wm_event_type_str_static(MC_WMEventType type){
    switch (type){
    #define MC_EVENT(NAME, GROUP, SERIAL) case MC_WME_##NAME: return #SERIAL;
        MC_ITER_WM_EVENTS()
    #undef MC_EVENT
    default:
        return mc_wm_event_type_group(type) == MC_WME_GROUP_USER ? "USER.UNKNOWN" : NULL;
    }
}

inline MC_WMEventType mc_wm_event_type_from_str_static(const char *name){
    #define MC_EVENT(NAME, GROUP, SERIAL) if(strcmp(name, #SERIAL) == 0){ return MC_WME_##NAME; }
        MC_ITER_WM_EVENTS()
    #undef MC_EVENT

    return MC_WME_NONE;
}

typedef struct MC_WMUserEventDef{
    const char *name;
} MC_WMUserEventDef;

MC_Alloc *mc_wm_event_allocator(MC_WMRef *wm);

MC_Error mc_wm_register_user_event(MC_WMRef *wm, const char *subgroup,
    const MC_WMUserEventDef *events, size_t count, MC_WMEventType *out_offset);
MC_Error mc_wm_user_event(MC_WMRef *wm, MC_WMEventType type, MC_WMEvent *out);
const char *mc_wm_event_type_str(MC_WMRef *wm, MC_WMEventType type);
MC_WMEventType mc_wm_event_type_from_str(MC_WMRef *wm, const char *name);

typedef struct MC_WMWindowEvent{
    uint64_t window;
} MC_WMWindowEvent;

struct MC_WMEvent{
    MC_WMEventType type;
    union {
        void *raw;

        struct MC_WME_User{
            MC_WMEventType offset;
            MC_Json *data;
        } user;

        MC_WMWindowEvent window;

        struct MC_WME_WindowReady{
            MC_WMWindowEvent base;
        } window_ready;

        struct MC_WME_WindowResized{
            MC_WMWindowEvent base;
            MC_Size2U new_size;
        } window_resized;

        struct MC_WME_WindowMoved{
            MC_WMWindowEvent base;
            MC_Vec2i new_position;
        } window_moved;

        struct MC_WME_RedrawRequest{
            MC_WMWindowEvent base;
        } redraw_requested;

        struct MC_WME_WindowStateChanged{
            MC_WMWindowEvent base;
            MC_WMWindowState state;
        } window_state_changed;

        struct MC_WME_WindowCloseRequest{
            MC_WMWindowEvent base;
        } window_close_requested;

        struct MC_WME_FocusGained{
            MC_WMWindowEvent base;
        } focus_gained;

        struct MC_WME_FocusLost{
            MC_WMWindowEvent base;
        } focus_lost;

        struct MC_WME_MouseMoved{
            MC_WMWindowEvent base;
            MC_Vec2i position;
        } mouse_moved;

        struct MC_WME_MouseDown{
            MC_WMWindowEvent base;
            MC_Vec2i position;
            MC_MouseButton button;
        } mouse_down;

        struct MC_WME_MouseUp{
            MC_WMWindowEvent base;
            MC_Vec2i position;
            MC_MouseButton button;
        } mouse_up;

        struct MC_WME_MouseEnter{
            MC_WMWindowEvent base;
            MC_Vec2i position;
        } mouse_enter;

        struct MC_WME_MouseLeave{
            MC_WMWindowEvent base;
            MC_Vec2i position;
        } mouse_leave;

        struct MC_WME_MouseWheel{
            MC_WMWindowEvent base;
            MC_Vec2i position;
            int up;
            int right;
        } mouse_wheel;

        struct MC_WME_KeyDown{
            MC_WMWindowEvent base;
            MC_Key key;
        } key_down;

        struct MC_WME_KeyUp{
            MC_WMWindowEvent base;
            MC_Key key;
        } key_up;

        struct MC_WME_TextInput{
            MC_WMWindowEvent base;
            char utf8[MC_WM_TEXT_INPUT_CAP];
        } text_input;

        struct MC_WME_PasteText{
            MC_WMWindowEvent base;
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

MC_Error mc_wm_event_to_json(MC_WMRef *wm, MC_Alloc *alloc, const MC_WMEvent *event, MC_Json **out);

#endif // MC_WM_EVENT_H
