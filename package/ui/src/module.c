#include <mc/ui/ui.h>

#include <mc/util/error.h>
#include <mc/data/json.h>
#include <mc/data/alloc.h>

#include <stdint.h>

enum {
    UI_TYPE_I32,
    UI_TYPE_U32,
};

static MC_Error ui_from_json(MC_UITypeID type, const MC_Json *json, MC_Alloc *alloc, void **raw) {
    (void)alloc;

    switch (type) {
    case UI_TYPE_I32: {
        int64_t value = 0;
        MC_RETURN_ERROR(mc_json_i64((MC_Json*)json, &value));

        *raw = (void*)(intptr_t)(int32_t)value;
        return MCE_OK;
    }
    case UI_TYPE_U32: {
        uint64_t value = 0;
        MC_RETURN_ERROR(mc_json_u64((MC_Json*)json, &value));

        *raw = (void*)(uintptr_t)(uint32_t)value;
        return MCE_OK;
    }
    default:
        return MCE_NOT_SUPPORTED;
    }
}

static MC_Error ui_to_json(MC_UITypeID type, const void *raw, MC_Alloc *alloc, MC_Json **json) {
    MC_Json *out = NULL;
    MC_RETURN_ERROR(mc_json_new(alloc, &out));

    MC_Error error;
    switch (type) {
    case UI_TYPE_I32: error = mc_json_set_i64(out, (int32_t)(intptr_t)raw); break;
    case UI_TYPE_U32: error = mc_json_set_u64(out, (uint32_t)(uintptr_t)raw); break;
    default: error = MCE_NOT_SUPPORTED; break;
    }

    if (error != MCE_OK) {
        mc_json_delete(&out);
        return error;
    }

    *json = out;
    return MCE_OK;
}

const MC_UIModuleDef mc_ui_module_def = {
    .name = "ui",
    .views = (MC_UIViewDef[]){
        {
            .name = "View",
            .ctor = NULL,
            .dtor = NULL,
            .events = (char *[]){
                "resize",
                "move",
                NULL
            },
            .props = (char *[]){
                "size",
                "position",
                NULL
            },
        },
        {}
    },
    .types = (char *[]){
        "i32",
        "u32",
        NULL
    },
    .value_from_json = ui_from_json,
    .value_to_json = ui_to_json,
};
