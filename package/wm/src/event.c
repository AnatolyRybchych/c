#include <mc/wm/event.h>

#include <mc/util/error.h>
#include <mc/data/str.h>

#include <stdbool.h>
#include <string.h>

static const char *window_state_str(MC_WMWindowState state){
    switch(state){
    case MC_WM_WINDOW_STATE_NORMAL: return "normal";
    case MC_WM_WINDOW_STATE_MINIMIZED: return "minimized";
    case MC_WM_WINDOW_STATE_MAXIMIZED: return "maximized";
    case MC_WM_WINDOW_STATE_FULLSCREEN: return "fullscreen";
    default: return "normal";
    }
}

static MC_WMWindowState parse_state(MC_Str s){
    if(mc_str_equ(s, mc_strc("minimized"))){ return MC_WM_WINDOW_STATE_MINIMIZED; }
    if(mc_str_equ(s, mc_strc("maximized"))){ return MC_WM_WINDOW_STATE_MAXIMIZED; }
    if(mc_str_equ(s, mc_strc("fullscreen"))){ return MC_WM_WINDOW_STATE_FULLSCREEN; }
    return MC_WM_WINDOW_STATE_NORMAL;
}

static MC_MouseButton parse_button(MC_Str s){
    #define MOUSE_BUTTON(NAME, ...) if(mc_str_equ(s, mc_strc(#NAME))){ return MC_MOUSE_##NAME; }
    MC_ITER_MOUSE_BUTTONS()
    #undef MOUSE_BUTTON
    return MC_MOUSE_UNKNOWN;
}

static MC_Error add_u64(MC_Json *obj, const char *key, uint64_t value){
    return mc_json_object_add_u64(obj, value, "%s", key);
}

static MC_Error add_i64(MC_Json *obj, const char *key, int64_t value){
    return mc_json_object_add_i64(obj, value, "%s", key);
}

static MC_Error add_cstr(MC_Json *obj, const char *key, const char *value){
    return mc_json_object_add_str(obj, mc_strc(value), "%s", key);
}

static MC_Error add_str(MC_Json *obj, const char *key, MC_Str value){
    return mc_json_object_add_str(obj, value, "%s", key);
}

static MC_Error add_position(MC_Json *obj, const char *key, MC_Vec2i position){
    MC_Json *item;
    MC_RETURN_ERROR(mc_json_object_add_new(obj, &item, "%s", key));
    MC_RETURN_ERROR(mc_json_set_object(item));

    MC_RETURN_ERROR(add_i64(item, "x", position.x));
    return add_i64(item, "y", position.y);
}

static MC_Error add_size(MC_Json *obj, const char *key, MC_Size2U size){
    MC_Json *item;
    MC_RETURN_ERROR(mc_json_object_add_new(obj, &item, "%s", key));
    MC_RETURN_ERROR(mc_json_set_object(item));

    MC_RETURN_ERROR(add_u64(item, "width", size.width));
    return add_u64(item, "height", size.height);
}

static MC_Error event_fields(MC_Json *obj, const MC_WMEvent *e){
    switch(e->type){
    case MC_WME_WINDOW_READY:
    case MC_WME_WINDOW_REDRAW_REQUESTED:
    case MC_WME_WINDOW_CLOSE_REQUESTED:
    case MC_WME_FOCUS_GAINED:
    case MC_WME_FOCUS_LOST:
    case MC_WME_MOUSE_CLICK:
        return add_u64(obj, "window", e->as.window);
    case MC_WME_WINDOW_RESIZED:
        MC_RETURN_ERROR(add_u64(obj, "window", e->as.window));
        return add_size(obj, "new_size", e->as.window_resized.new_size);
    case MC_WME_WINDOW_MOVED:
        MC_RETURN_ERROR(add_u64(obj, "window", e->as.window));
        return add_position(obj, "new_position", e->as.window_moved.new_position);
    case MC_WME_WINDOW_STATE_CHANGED:
        MC_RETURN_ERROR(add_u64(obj, "window", e->as.window));
        return add_cstr(obj, "state", window_state_str(e->as.window_state_changed.state));
    case MC_WME_MOUSE_MOVED:
        MC_RETURN_ERROR(add_u64(obj, "window", e->as.window));
        return add_position(obj, "position", e->as.mouse_moved.position);
    case MC_WME_MOUSE_DOWN:
        MC_RETURN_ERROR(add_u64(obj, "window", e->as.window));
        MC_RETURN_ERROR(add_position(obj, "position", e->as.mouse_down.position));
        return add_cstr(obj, "button", mc_mouse_button_str(e->as.mouse_down.button));
    case MC_WME_MOUSE_UP:
        MC_RETURN_ERROR(add_u64(obj, "window", e->as.window));
        MC_RETURN_ERROR(add_position(obj, "position", e->as.mouse_up.position));
        return add_cstr(obj, "button", mc_mouse_button_str(e->as.mouse_up.button));
    case MC_WME_MOUSE_ENTER:
        MC_RETURN_ERROR(add_u64(obj, "window", e->as.window));
        return add_position(obj, "position", e->as.mouse_enter.position);
    case MC_WME_MOUSE_LEAVE:
        MC_RETURN_ERROR(add_u64(obj, "window", e->as.window));
        return add_position(obj, "position", e->as.mouse_leave.position);
    case MC_WME_MOUSE_WHEEL:
        MC_RETURN_ERROR(add_u64(obj, "window", e->as.window));
        MC_RETURN_ERROR(add_position(obj, "position", e->as.mouse_wheel.position));
        MC_RETURN_ERROR(add_i64(obj, "up", e->as.mouse_wheel.up));
        return add_i64(obj, "right", e->as.mouse_wheel.right);
    case MC_WME_KEY_DOWN:
        MC_RETURN_ERROR(add_u64(obj, "window", e->as.window));
        return add_cstr(obj, "key", mc_key_str(e->as.key_down.key));
    case MC_WME_KEY_UP:
        MC_RETURN_ERROR(add_u64(obj, "window", e->as.window));
        return add_cstr(obj, "key", mc_key_str(e->as.key_up.key));
    case MC_WME_TEXT_INPUT:
        MC_RETURN_ERROR(add_u64(obj, "window", e->as.window));
        return add_cstr(obj, "text", e->as.text_input.utf8);
    case MC_WME_PASTE_TEXT:
        MC_RETURN_ERROR(add_u64(obj, "window", e->as.window));
        return add_str(obj, "text", e->as.paste_text.text);
    case MC_WME_GLOBAL_KEY_DOWN:
        return add_cstr(obj, "key", mc_key_str(e->as.global_key_down.key));
    case MC_WME_GLOBAL_KEY_UP:
        return add_cstr(obj, "key", mc_key_str(e->as.global_key_up.key));
    case MC_WME_GLOBAL_MOUSE_MOVED:
        return add_position(obj, "position", e->as.global_mouse_moved.position);
    case MC_WME_GLOBAL_MOUSE_DOWN:
        MC_RETURN_ERROR(add_position(obj, "position", e->as.global_mouse_down.position));
        return add_cstr(obj, "button", mc_mouse_button_str(e->as.global_mouse_down.button));
    case MC_WME_GLOBAL_MOUSE_UP:
        MC_RETURN_ERROR(add_position(obj, "position", e->as.global_mouse_up.position));
        return add_cstr(obj, "button", mc_mouse_button_str(e->as.global_mouse_up.button));
    case MC_WME_GLOBAL_MOUSE_WHEEL:
        MC_RETURN_ERROR(add_position(obj, "position", e->as.global_mouse_wheel.position));
        MC_RETURN_ERROR(add_i64(obj, "up", e->as.global_mouse_wheel.up));
        return add_i64(obj, "right", e->as.global_mouse_wheel.right);
    default:
        return MCE_OK;
    }
}

static MC_Error wm_group_to_json(MC_Alloc *alloc, const MC_WMEvent *event, MC_Json *json){
    (void)alloc;
    return event_fields(json, event);
}

static MC_Vec2i read_pos(MC_Json *node){
    MC_Json *x = node != NULL ? mc_json_field(node, "x") : NULL;
    MC_Json *y = node != NULL ? mc_json_field(node, "y") : NULL;
    return (MC_Vec2i){
        .x = x != NULL ? (int)mc_json_as_i64(x) : 0,
        .y = y != NULL ? (int)mc_json_as_i64(y) : 0,
    };
}

static MC_Size2U read_size(MC_Json *node){
    MC_Json *w = node != NULL ? mc_json_field(node, "width") : NULL;
    MC_Json *h = node != NULL ? mc_json_field(node, "height") : NULL;
    return (MC_Size2U){
        .width = w != NULL ? (unsigned)mc_json_as_u64(w) : 0,
        .height = h != NULL ? (unsigned)mc_json_as_u64(h) : 0,
    };
}

static MC_Key read_key(const MC_Json *json){
    MC_Json *k = mc_json_field(json, "key");
    MC_Str s;
    return (k != NULL && mc_json_str(k, &s) == MCE_OK) ? mc_key_from_str(s) : MC_KEY_UNKNOWN;
}

static MC_MouseButton read_button(const MC_Json *json){
    MC_Json *b = mc_json_field(json, "button");
    MC_Str s;
    return (b != NULL && mc_json_str(b, &s) == MCE_OK) ? parse_button(s) : MC_MOUSE_UNKNOWN;
}

static void read_window(const MC_Json *json, MC_WMEvent *event){
    MC_Json *w = mc_json_field(json, "window");
    if(w != NULL){
        event->as.window = mc_json_as_u64(w);
    }
}

static MC_Error wm_group_from_json(MC_Alloc *alloc, const MC_Json *json, MC_WMEvent *event){
    switch(event->type){
    case MC_WME_WINDOW_READY:
    case MC_WME_WINDOW_REDRAW_REQUESTED:
    case MC_WME_WINDOW_CLOSE_REQUESTED:
    case MC_WME_FOCUS_GAINED:
    case MC_WME_FOCUS_LOST:
    case MC_WME_MOUSE_CLICK:
        read_window(json, event);
        break;
    case MC_WME_WINDOW_RESIZED:
        read_window(json, event);
        event->as.window_resized.new_size = read_size(mc_json_field(json, "new_size"));
        break;
    case MC_WME_WINDOW_MOVED:
        read_window(json, event);
        event->as.window_moved.new_position = read_pos(mc_json_field(json, "new_position"));
        break;
    case MC_WME_WINDOW_STATE_CHANGED:{
        read_window(json, event);
        MC_Json *s = mc_json_field(json, "state");
        MC_Str ss;
        if(s != NULL && mc_json_str(s, &ss) == MCE_OK){
            event->as.window_state_changed.state = parse_state(ss);
        }
        break;
    }
    case MC_WME_MOUSE_MOVED:
        read_window(json, event);
        event->as.mouse_moved.position = read_pos(mc_json_field(json, "position"));
        break;
    case MC_WME_MOUSE_DOWN:
        read_window(json, event);
        event->as.mouse_down.position = read_pos(mc_json_field(json, "position"));
        event->as.mouse_down.button = read_button(json);
        break;
    case MC_WME_MOUSE_UP:
        read_window(json, event);
        event->as.mouse_up.position = read_pos(mc_json_field(json, "position"));
        event->as.mouse_up.button = read_button(json);
        break;
    case MC_WME_MOUSE_ENTER:
        read_window(json, event);
        event->as.mouse_enter.position = read_pos(mc_json_field(json, "position"));
        break;
    case MC_WME_MOUSE_LEAVE:
        read_window(json, event);
        event->as.mouse_leave.position = read_pos(mc_json_field(json, "position"));
        break;
    case MC_WME_MOUSE_WHEEL:
        read_window(json, event);
        event->as.mouse_wheel.position = read_pos(mc_json_field(json, "position"));
        event->as.mouse_wheel.up = (int)mc_json_as_i64(mc_json_field(json, "up"));
        event->as.mouse_wheel.right = (int)mc_json_as_i64(mc_json_field(json, "right"));
        break;
    case MC_WME_KEY_DOWN:
        read_window(json, event);
        event->as.key_down.key = read_key(json);
        break;
    case MC_WME_KEY_UP:
        read_window(json, event);
        event->as.key_up.key = read_key(json);
        break;
    case MC_WME_TEXT_INPUT:{
        read_window(json, event);
        MC_Json *t = mc_json_field(json, "text");
        MC_Str ts;
        if(t != NULL && mc_json_str(t, &ts) == MCE_OK){
            size_t len = (size_t)MC_STR_LEN(ts);
            if(len >= MC_WM_TEXT_INPUT_CAP){
                len = MC_WM_TEXT_INPUT_CAP - 1;
            }
            memcpy(event->as.text_input.utf8, ts.beg, len);
            event->as.text_input.utf8[len] = '\0';
        }
        break;
    }
    case MC_WME_PASTE_TEXT:{
        read_window(json, event);
        MC_Json *t = mc_json_field(json, "text");
        MC_Str ts;
        event->as.paste_text.text = MC_STR(NULL, NULL);
        if(t != NULL && mc_json_str(t, &ts) == MCE_OK){
            size_t len = (size_t)MC_STR_LEN(ts);
            char *copy = NULL;
            if(len > 0 && mc_alloc(alloc, len, (void**)&copy) == MCE_OK){
                memcpy(copy, ts.beg, len);
                event->as.paste_text.text = MC_STR(copy, copy + len);
            }
        }
        break;
    }
    case MC_WME_GLOBAL_KEY_DOWN:
        event->as.global_key_down.key = read_key(json);
        break;
    case MC_WME_GLOBAL_KEY_UP:
        event->as.global_key_up.key = read_key(json);
        break;
    case MC_WME_GLOBAL_MOUSE_MOVED:
        event->as.global_mouse_moved.position = read_pos(mc_json_field(json, "position"));
        break;
    case MC_WME_GLOBAL_MOUSE_DOWN:
        event->as.global_mouse_down.position = read_pos(mc_json_field(json, "position"));
        event->as.global_mouse_down.button = read_button(json);
        break;
    case MC_WME_GLOBAL_MOUSE_UP:
        event->as.global_mouse_up.position = read_pos(mc_json_field(json, "position"));
        event->as.global_mouse_up.button = read_button(json);
        break;
    case MC_WME_GLOBAL_MOUSE_WHEEL:
        event->as.global_mouse_wheel.position = read_pos(mc_json_field(json, "position"));
        event->as.global_mouse_wheel.up = (int)mc_json_as_i64(mc_json_field(json, "up"));
        event->as.global_mouse_wheel.right = (int)mc_json_as_i64(mc_json_field(json, "right"));
        break;
    default:
        break;
    }

    return MCE_OK;
}

static const MC_WMEventDefinition wm_event_defs[] = {
#define MC_EVENT(NAME, SERIAL) { .name = #SERIAL },
    MC_ITER_WM_EVENTS()
#undef MC_EVENT
};

const MC_WMEventGroupDef *mc_wm_builtin_group_def(void){
    static const MC_WMEventGroupDef def = {
        .name = "WM",
        .events = wm_event_defs,
        .size = MC_WME_WM_COUNT,
        .reserve = 0,
        .to_json = wm_group_to_json,
        .from_json = wm_group_from_json,
    };

    return &def;
}
