#include <mc/ui/ui.h>

#include <mc/util/error.h>
#include <mc/data/vector.h>
#include <mc/data/string.h>
#include <mc/data/stack.h>

typedef struct ModuleInfo ModuleInfo;
typedef struct ViewInfo ViewInfo;
typedef struct EventInfo EventInfo;
typedef struct PropInfo PropInfo;

struct ModuleInfo {
    MC_String *name;
    MC_UIModuleID id;
    unsigned views_cnt;
    unsigned reserved_cnt;
};

struct ViewInfo {
    MC_String *name;
    MC_UIViewID id;
    unsigned events_cnt;
    unsigned props_cnt;
    unsigned reserved_cnt;
    MC_Error (*event_to_json)(const MC_UiEvent *event, MC_Alloc *alloc, MC_Json **json);
    MC_Error (*event_from_json)(const MC_Json *json, MC_UIProp prop, MC_UiEvent **event);
    MC_Error (*prop_to_json)(const MC_UIView *view, MC_UIProp prop, MC_Alloc *alloc, MC_Json **json);
    MC_Error (*prop_from_json)(MC_UIView *view, MC_UIProp prop, const MC_Json *json);
};

struct EventInfo {
    unsigned id;
    MC_String *type;
};

struct PropInfo {
    unsigned id;
    MC_String *name;
};

MC_DEFINE_VECTOR(ModuleInfoList, ModuleInfo);
MC_DEFINE_VECTOR(ViewInfoList, ViewInfo);
MC_DEFINE_VECTOR(EventInfoList, EventInfo);
MC_DEFINE_VECTOR(PropInfoList, PropInfo);

struct MC_UI {
    MC_Alloc *alloc;
    MC_Stack *stack;

    ModuleInfoList *modules;
    ViewInfoList *views;
    PropInfoList *props;
    EventInfoList *events;
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

    MC_VECTOR_FREE(ui->modules);
    MC_VECTOR_FREE(ui->views);
    MC_VECTOR_FREE(ui->events);
    MC_VECTOR_FREE(ui->props);

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

MC_UIView *mc_ui_find_view(const MC_UI *ui, const char *name) {
    (void)ui;
    (void)name;
    return NULL;
}

static MC_Error register_module(MC_UI *ui, const MC_UIModuleDef *def) {
    unsigned views_cnt = 0;
    unsigned module_reserved_cnt = 0;

    for (const MC_UIViewDef *view = def->views; view && view->name; view++) {
        views_cnt += 1;

        unsigned props_cnt = 0;
        for (char *const *prop = view->props; prop && *prop; prop++) {
            props_cnt += 1;
        }
        
        unsigned events_cnt = 0;
        for (char *const *events = view->events; events && *events; events++) {
            events_cnt += 1;
        }

        unsigned reserved_cnt = MAX(events_cnt, props_cnt);
        module_reserved_cnt += reserved_cnt;
    }

    ModuleInfoList *new_modules = MC_VECTOR_RESERVE(ui->modules, 1);
    ViewInfoList *new_views = MC_VECTOR_RESERVE(ui->views, views_cnt);
    EventInfoList *new_events = MC_VECTOR_RESERVE(ui->events, module_reserved_cnt);
    PropInfoList *new_props = MC_VECTOR_RESERVE(ui->props, module_reserved_cnt);

    ui->modules = new_modules ? new_modules : ui->modules;
    ui->views = new_views ? new_views : ui->views;
    ui->events = new_events ? new_events : ui->events;
    ui->props = new_props ? new_props : ui->props;

    if (!new_modules || !new_views || !new_events || !new_props) {
        return MCE_OUT_OF_MEMORY;
    }

    memset(ui->modules->end, 0, sizeof(ModuleInfo[1]));
    memset(ui->views->end, 0, sizeof(ViewInfo[views_cnt]));
    memset(ui->events->end, 0, sizeof(EventInfo[module_reserved_cnt]));
    memset(ui->props->end, 0, sizeof(PropInfo[module_reserved_cnt]));

    ModuleInfo module = (ModuleInfo){
        .id = MC_VECTOR_SIZE(ui->props),
        .reserved_cnt = module_reserved_cnt,
        .views_cnt = views_cnt,
    };

    *ui->modules->end = module;

    MC_UIViewID view_id = module.id;
    unsigned view_idx = 0;
    for (const MC_UIViewDef *view_def = def->views; view_def && view_def->name; view_def++) {
        ViewInfo view = (ViewInfo) {
            .id = view_id
        };
        
        MC_RETURN_ERROR(mc_string(ui->alloc, &view.name, mc_strc(view_def->name)));

        for (char *const *prop_name = view_def->props; prop_name && *prop_name; prop_name++) {
            PropInfo prop_info = (PropInfo) {
                .id = view.id + view.props_cnt
            };

            MC_RETURN_ERROR(mc_string(ui->alloc, &prop_info.name, mc_strc(*prop_name)));

            ui->props->beg[prop_info.id] = prop_info;
            view.props_cnt += 1;
        }

        for (char *const *event_name = view_def->events; event_name && *event_name; event_name++) {
            EventInfo event_info = (EventInfo) {
                .id = view.id + view.events_cnt
            };

            MC_RETURN_ERROR(mc_string(ui->alloc, &event_info.type, mc_strc(*event_name)));

            ui->events->beg[event_info.id] = event_info;
            view.events_cnt += 1;
        }

        view.reserved_cnt += MAX(view.props_cnt, view.events_cnt);
        ui->views->end[view_idx] = view;
        view_idx += 1;
        view_id += view.reserved_cnt;
    }

    ui->modules->end += 1;
    ui->views->end += module.views_cnt;

    // its a little bit redundant to make the view id as a base for both properties and events
    ui->props->end += module.reserved_cnt;
    ui->events->end += module.reserved_cnt;

    return MCE_OK;
}