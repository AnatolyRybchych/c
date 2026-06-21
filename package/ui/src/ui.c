#include <mc/ui/ui.h>
#include <mc/ui/metadata.h>

#include <mc/util/error.h>
#include <mc/data/vector.h>
#include <mc/data/string.h>
#include <mc/data/str.h>
#include <mc/data/stack.h>

#include <stddef.h>
#include <string.h>

typedef struct UiElementData UiElementData;
typedef struct UIElementInstance UIElementInstance;

struct UiElementData {
    UiElementData *parent;

    max_align_t data[];
};

struct UIElementInstance {
    MC_UIViewID view;
    UiElementData *data;
};

struct MC_UIElement {
    MC_UI *ui;
    UIElementInstance instance;
};

MC_DEFINE_VECTOR(ElementInstanceList, UIElementInstance);

struct MC_UI {
    MC_Alloc *alloc;
    MC_Stack *stack;
    MC_UIInfo metadata;
    ElementInstanceList *elements;
};

static MC_Error register_module(MC_UI *ui, const MC_UIModuleDef *def);

MC_Error mc_ui_init(MC_Alloc *base_alloc, MC_UI **ui) {
    MC_Stack *stack = NULL;
    MC_RETURN_ERROR(mc_stack_init(&stack, base_alloc));

    MC_Alloc *alloc = mc_stack_allocator(stack);

    MC_UI *out = NULL;
    MC_RETURN_ERROR(mc_alloc(alloc, sizeof *out, (void**)&out));

    *out = (MC_UI) {
        .alloc = alloc,
        .stack = stack,
    };

    *ui = out;
    return MCE_OK;
}

void mc_ui_destroy(MC_UI *ui) {
    if (ui == NULL){
        return;
    }

    MC_VECTOR_FREE(NULL, ui->metadata.modules);
    MC_VECTOR_FREE(NULL, ui->metadata.views);
    MC_VECTOR_FREE(NULL, ui->metadata.events);
    MC_VECTOR_FREE(NULL, ui->metadata.props);
    MC_VECTOR_FREE(NULL, ui->elements);

    mc_stack_destroy(ui->stack);
}

MC_Error mc_ui_register_module(MC_UI *ui, const MC_UIModuleDef *def) {
    MC_StackCursor top = mc_stack_top(ui->stack);
    MC_Error error = register_module(ui, def);
    if (error) {
        mc_stack_settop(ui->stack, top);
    }
    return error;
}

MC_UIViewID mc_ui_find_view(const MC_UI *ui, const char *name) {
    if (ui == NULL || name == NULL) {
        return MC_UI_VIEW_INVALID;
    }

    MC_Str target = mc_strc(name);
    for (size_t i = 0; i < (size_t)MC_VECTOR_SIZE(ui->metadata.views); i++) {
        if (mc_str_equ(mc_string_str(ui->metadata.views->beg[i].name), target)) {
            return ui->metadata.views->beg[i].id;
        }
    }

    return MC_UI_VIEW_INVALID;
}

MC_Error mc_ui_create_element(MC_UI *ui, MC_UIViewID view, MC_UIElementID *out) {
    MC_RETURN_INVALID(ui == NULL || out == NULL);
    if (view >= MC_VECTOR_SIZE(ui->metadata.views)) {
        return MCE_NOT_FOUND;
    }
    MC_UIViewInfo *info = &ui->metadata.views->beg[view];

    MC_StackCursor top = mc_stack_top(ui->stack);

    UiElementData *data = NULL;
    MC_RETURN_ERROR(mc_alloc(ui->alloc, sizeof(UiElementData) + info->size, (void**)&data));
    data->parent = NULL;
    memset(data->data, 0, info->size);

    UIElementInstance instance = { .view = view, .data = data };

    MC_UIElementID id = MC_VECTOR_SIZE(ui->elements);
    MC_Error error = MC_VECTOR_PUSHN(NULL, ui->elements, 1, &instance);
    if (error != MCE_OK) {
        mc_stack_settop(ui->stack, top);
        return error;
    }

    if (info->ctor != NULL) {
        MC_UIElement element = { .ui = ui, .instance = instance };
        info->ctor(&element);
    }

    *out = id;
    return MCE_OK;
}

MC_Error mc_ui_element_destroy(MC_UI *ui, MC_UIElementID element) {
    if (ui == NULL || element >= MC_VECTOR_SIZE(ui->elements)) {
        return MCE_INVALID_INPUT;
    }

    UIElementInstance instance = ui->elements->beg[element];

    UiElementData *data = instance.data;
    for (MC_UIViewID v = instance.view; v != MC_UI_VIEW_INVALID && data != NULL; v = ui->metadata.views->beg[v].parent) {
        MC_UIViewInfo *info = &ui->metadata.views->beg[v];
        if (info->dtor != NULL) {
            MC_UIElement el = {
                .ui = ui,
                .instance = { .view = v, .data = data },
            };
            info->dtor(&el);
        }

        data = data->parent;
    }

    return MCE_OK;
}

static MC_Error register_module(MC_UI *ui, const MC_UIModuleDef *def) {
    unsigned views_cnt = 0;
    unsigned module_props_cnt = 0;
    unsigned module_events_cnt = 0;

    for (const MC_UIViewDef *view = def->views; view && view->name; view++) {
        views_cnt += 1;

        for (char *const *prop = view->props; prop && *prop; prop++) {
            module_props_cnt += 1;
        }

        for (char *const *event = view->events; event && *event; event++) {
            module_events_cnt += 1;
        }
    }

    MC_RETURN_ERROR(MC_VECTOR_RESERVE(NULL, ui->metadata.modules, 1));
    MC_RETURN_ERROR(MC_VECTOR_RESERVE(NULL, ui->metadata.views, views_cnt));
    MC_RETURN_ERROR(MC_VECTOR_RESERVE(NULL, ui->metadata.events, module_events_cnt));
    MC_RETURN_ERROR(MC_VECTOR_RESERVE(NULL, ui->metadata.props, module_props_cnt));

    memset(ui->metadata.modules->end, 0, sizeof(MC_UIModuleInfo[1]));
    memset(ui->metadata.views->end, 0, sizeof(MC_UIViewInfo[views_cnt]));
    memset(ui->metadata.events->end, 0, sizeof(MC_UIEventInfo[module_events_cnt]));
    memset(ui->metadata.props->end, 0, sizeof(MC_UIPropInfo[module_props_cnt]));

    MC_UIModuleInfo module = (MC_UIModuleInfo){
        .id = MC_VECTOR_SIZE(ui->metadata.modules),
        .views_cnt = views_cnt,
        .prop_base = MC_VECTOR_SIZE(ui->metadata.props),
        .event_base = MC_VECTOR_SIZE(ui->metadata.events),
        .props_cnt = module_props_cnt,
        .events_cnt = module_events_cnt,
    };

    *ui->metadata.modules->end = module;

    unsigned views_base = MC_VECTOR_SIZE(ui->metadata.views);
    unsigned prop_base = module.prop_base;
    unsigned event_base = module.event_base;
    unsigned view_idx = 0;
    for (const MC_UIViewDef *view_def = def->views; view_def && view_def->name; view_def++) {
        MC_UIViewInfo view = (MC_UIViewInfo) {
            .id = views_base + view_idx,
            .parent = MC_UI_VIEW_INVALID,
            .size = view_def->size,
            .prop_base = prop_base,
            .event_base = event_base,
            .ctor = view_def->ctor,
            .dtor = view_def->dtor,
            .handle_event = view_def->handle_event,
            .prop_to_json = view_def->prop_to_json,
            .prop_from_json = view_def->prop_from_json,
            .event_to_json = view_def->event_to_json,
            .event_from_json = view_def->event_from_json,
        };

        MC_RETURN_ERROR(mc_string(ui->alloc, &view.name, mc_strc(view_def->name)));

        for (char *const *prop_name = view_def->props; prop_name && *prop_name; prop_name++) {
            MC_UIPropInfo prop_info = (MC_UIPropInfo) {
                .id = view.prop_base + view.props_cnt
            };

            MC_RETURN_ERROR(mc_string(ui->alloc, &prop_info.name, mc_strc(*prop_name)));

            ui->metadata.props->beg[prop_info.id] = prop_info;
            view.props_cnt += 1;
        }

        for (char *const *event_name = view_def->events; event_name && *event_name; event_name++) {
            MC_UIEventInfo event_info = (MC_UIEventInfo) {
                .id = view.event_base + view.events_cnt
            };

            MC_RETURN_ERROR(mc_string(ui->alloc, &event_info.type, mc_strc(*event_name)));

            ui->metadata.events->beg[event_info.id] = event_info;
            view.events_cnt += 1;
        }

        ui->metadata.views->end[view_idx] = view;
        view_idx += 1;
        prop_base += view.props_cnt;
        event_base += view.events_cnt;
    }

    ui->metadata.modules->end += 1;
    ui->metadata.views->end += module.views_cnt;
    ui->metadata.props->end += module.props_cnt;
    ui->metadata.events->end += module.events_cnt;

    unsigned resolve_idx = 0;
    for (const MC_UIViewDef *view_def = def->views; view_def && view_def->name; view_def++) {
        if (view_def->parent != NULL) {
            ui->metadata.views->beg[views_base + resolve_idx].parent = mc_ui_find_view(ui, view_def->parent);
        }
        resolve_idx += 1;
    }

    return MCE_OK;
}
