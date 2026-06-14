#include <mc/wm/event.h>

#include <mc/util/error.h>

extern inline MC_WMEventGroup mc_wm_event_type_group(MC_WMEventType type);

static const char *window_state_str(MC_WMWindowState state){
    switch(state){
    case MC_WM_WINDOW_STATE_NORMAL: return "normal";
    case MC_WM_WINDOW_STATE_MINIMIZED: return "minimized";
    case MC_WM_WINDOW_STATE_MAXIMIZED: return "maximized";
    case MC_WM_WINDOW_STATE_FULLSCREEN: return "fullscreen";
    default: return "normal";
    }
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
    case MC_WME_WINDOW_RESIZED:
        return add_size(obj, "new_size", e->as.window_resized.new_size);
    case MC_WME_WINDOW_MOVED:
        return add_position(obj, "new_position", e->as.window_moved.new_position);
    case MC_WME_WINDOW_STATE_CHANGED:
        return add_cstr(obj, "state", window_state_str(e->as.window_state_changed.state));
    case MC_WME_MOUSE_MOVED:
        return add_position(obj, "position", e->as.mouse_moved.position);
    case MC_WME_MOUSE_DOWN:
        MC_RETURN_ERROR(add_position(obj, "position", e->as.mouse_down.position));
        return add_cstr(obj, "button", mc_mouse_button_str(e->as.mouse_down.button));
    case MC_WME_MOUSE_UP:
        MC_RETURN_ERROR(add_position(obj, "position", e->as.mouse_up.position));
        return add_cstr(obj, "button", mc_mouse_button_str(e->as.mouse_up.button));
    case MC_WME_MOUSE_ENTER:
        return add_position(obj, "position", e->as.mouse_enter.position);
    case MC_WME_MOUSE_LEAVE:
        return add_position(obj, "position", e->as.mouse_leave.position);
    case MC_WME_MOUSE_WHEEL:
        MC_RETURN_ERROR(add_position(obj, "position", e->as.mouse_wheel.position));
        MC_RETURN_ERROR(add_i64(obj, "up", e->as.mouse_wheel.up));
        return add_i64(obj, "right", e->as.mouse_wheel.right);
    case MC_WME_KEY_DOWN:
        return add_cstr(obj, "key", mc_key_str(e->as.key_down.key));
    case MC_WME_KEY_UP:
        return add_cstr(obj, "key", mc_key_str(e->as.key_up.key));
    case MC_WME_TEXT_INPUT:
        return add_cstr(obj, "text", e->as.text_input.utf8);
    case MC_WME_PASTE_TEXT:
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
        if(mc_wm_event_type_group(e->type) == MC_WME_GROUP_USER && e->as.user.data != NULL){
            MC_Json *data;
            MC_RETURN_ERROR(mc_json_object_add_new(obj, &data, "data"));
            return mc_json_copy(data, e->as.user.data);
        }

        return MCE_OK;
    }
}

MC_Error mc_wm_event_to_json(MC_WMRef *wm, MC_Alloc *alloc, const MC_WMEvent *event, MC_Json **out){
    if(event == NULL || out == NULL){
        return MCE_INVALID_INPUT;
    }

    MC_Json *obj;
    MC_RETURN_ERROR(mc_json_new(alloc, &obj));

    MC_Error status = mc_json_set_object(obj);
    if(status == MCE_OK){
        status = add_cstr(obj, "type", mc_wm_event_type_str(wm, event->type));
    }

    if(status == MCE_OK && mc_wm_event_type_group(event->type) == MC_WME_GROUP_WINDOW){
        status = add_u64(obj, "window", event->as.window.window);
    }

    if(status == MCE_OK){
        status = event_fields(obj, event);
    }

    if(status != MCE_OK){
        mc_json_delete(&obj);
        return status;
    }

    *out = obj;
    return MCE_OK;
}
