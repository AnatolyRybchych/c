#include <mc/ui/ui.h>

#include <mc/util/error.h>

struct MC_UI {
    MC_Alloc *alloc;
};

MC_Error mc_ui_init(MC_Alloc *alloc, MC_UI **ui) {
    MC_UI *out = NULL;
    MC_RETURN_ERROR(mc_alloc(alloc, sizeof *out, (void**)&out));

    *out = (MC_UI) {
        .alloc = alloc
    };

    *ui = out;
    return MCE_OK;
}

void mc_ui_destroy(MC_UI *ui) {
    if (ui == NULL){
        return;
    }

    mc_free(ui->alloc, ui);
}

MC_Error mc_ui_register_module(MC_UI *ui, const MC_UIModuleDef *def) {
    return MCE_NOT_IMPLEMENTED;
}

MC_UIView *mc_ui_find_view(const MC_UI *ui, const char *name) {
    return NULL;
}