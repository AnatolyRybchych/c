#ifndef MC_UI_H
#define MC_UI_H

#include <mc/ui/view.h>
#include <mc/ui/module.h>

#include <mc/data/alloc.h>
#include <mc/error.h>

typedef struct MC_UI MC_UI;

extern const MC_UIModuleDef mc_ui_module_def;

MC_Error mc_ui_init(MC_Alloc *alloc, MC_UI **ui);
void mc_ui_destroy(MC_UI *ui);

MC_Error mc_ui_register_module(MC_UI *ui, const MC_UIModuleDef *def);

MC_UIView *mc_ui_find_view(const MC_UI *ui, const char *name);

#endif // MC_UI_H
