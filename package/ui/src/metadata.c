#include <mc/ui/metadata.h>

#include <mc/util/error.h>
#include <mc/data/vector.h>
#include <mc/data/string.h>

#include <stddef.h>

static MC_Error put_str(MC_Json *obj, const char *key, MC_String *str) {
    if (str == NULL) {
        MC_Json *node = NULL;
        MC_RETURN_ERROR(mc_json_object_add_new(obj, &node, "%s", key));
        return mc_json_set_null(node);
    }

    return mc_json_object_add_str(obj, mc_string_str(str), "%s", key);
}

static MC_Error dump_module(const MC_UIModuleInfo *module, MC_Json *obj) {
    MC_RETURN_ERROR(put_str(obj, "name", module->name));
    MC_RETURN_ERROR(mc_json_object_add_u64(obj, module->id, "id"));
    MC_RETURN_ERROR(mc_json_object_add_u64(obj, module->prop_base, "prop_base"));
    MC_RETURN_ERROR(mc_json_object_add_u64(obj, module->event_base, "event_base"));
    MC_RETURN_ERROR(mc_json_object_add_u64(obj, module->views_cnt, "views_cnt"));
    MC_RETURN_ERROR(mc_json_object_add_u64(obj, module->props_cnt, "props_cnt"));
    MC_RETURN_ERROR(mc_json_object_add_u64(obj, module->events_cnt, "events_cnt"));

    return MCE_OK;
}

static MC_Error dump_view(const MC_UIViewInfo *view, MC_Json *obj) {
    MC_RETURN_ERROR(put_str(obj, "name", view->name));
    MC_RETURN_ERROR(mc_json_object_add_u64(obj, view->id, "id"));
    MC_RETURN_ERROR(mc_json_object_add_u64(obj, view->parent, "parent"));
    MC_RETURN_ERROR(mc_json_object_add_u64(obj, view->size, "size"));
    MC_RETURN_ERROR(mc_json_object_add_u64(obj, view->prop_base, "prop_base"));
    MC_RETURN_ERROR(mc_json_object_add_u64(obj, view->event_base, "event_base"));
    MC_RETURN_ERROR(mc_json_object_add_u64(obj, view->props_cnt, "props_cnt"));
    MC_RETURN_ERROR(mc_json_object_add_u64(obj, view->events_cnt, "events_cnt"));

    return MCE_OK;
}

static MC_Error dump_prop(const MC_UIPropInfo *prop, MC_Json *obj) {
    MC_RETURN_ERROR(mc_json_object_add_u64(obj, prop->id, "id"));
    MC_RETURN_ERROR(put_str(obj, "name", prop->name));

    return MCE_OK;
}

static MC_Error dump_event(const MC_UIEventInfo *event, MC_Json *obj) {
    MC_RETURN_ERROR(mc_json_object_add_u64(obj, event->id, "id"));
    MC_RETURN_ERROR(put_str(obj, "type", event->type));

    return MCE_OK;
}

static MC_Error add_list(MC_Json *root, const char *key, MC_Json **out) {
    MC_RETURN_ERROR(mc_json_object_add_new(root, out, "%s", key));

    return mc_json_set_list(*out);
}

static MC_Error add_object(MC_Json *list, MC_Json **out) {
    MC_RETURN_ERROR(mc_json_list_add_new(list, out));

    return mc_json_set_object(*out);
}

static MC_Error metadata_to_json(const MC_UIInfo *metadata, MC_Json *root) {
    MC_RETURN_ERROR(mc_json_set_object(root));

    MC_Json *modules = NULL;
    MC_RETURN_ERROR(add_list(root, "modules", &modules));
    for (size_t i = 0; i < (size_t)MC_VECTOR_SIZE(metadata->modules); i++) {
        MC_Json *obj = NULL;
        MC_RETURN_ERROR(add_object(modules, &obj));
        MC_RETURN_ERROR(dump_module(&metadata->modules->beg[i], obj));
    }

    MC_Json *views = NULL;
    MC_RETURN_ERROR(add_list(root, "views", &views));
    for (size_t i = 0; i < (size_t)MC_VECTOR_SIZE(metadata->views); i++) {
        MC_Json *obj = NULL;
        MC_RETURN_ERROR(add_object(views, &obj));
        MC_RETURN_ERROR(dump_view(&metadata->views->beg[i], obj));
    }

    MC_Json *props = NULL;
    MC_RETURN_ERROR(add_list(root, "props", &props));
    for (size_t i = 0; i < (size_t)MC_VECTOR_SIZE(metadata->props); i++) {
        MC_Json *obj = NULL;
        MC_RETURN_ERROR(add_object(props, &obj));
        MC_RETURN_ERROR(dump_prop(&metadata->props->beg[i], obj));
    }

    MC_Json *events = NULL;
    MC_RETURN_ERROR(add_list(root, "events", &events));
    for (size_t i = 0; i < (size_t)MC_VECTOR_SIZE(metadata->events); i++) {
        MC_Json *obj = NULL;
        MC_RETURN_ERROR(add_object(events, &obj));
        MC_RETURN_ERROR(dump_event(&metadata->events->beg[i], obj));
    }

    return MCE_OK;
}

MC_Error mc_ui_metadata_to_json(const MC_UIInfo *metadata, MC_Alloc *alloc, MC_Json **out) {
    MC_RETURN_INVALID(metadata == NULL || out == NULL);

    MC_Json *root = NULL;
    MC_RETURN_ERROR(mc_json_new(alloc, &root));

    MC_Error error = metadata_to_json(metadata, root);
    if (error != MCE_OK) {
        mc_json_delete(&root);
        return error;
    }

    *out = root;
    return MCE_OK;
}
