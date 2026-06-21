#ifndef MC_UI_MODULE_H
#define MC_UI_MODULE_H

#include <mc/ui/view.h>

#include <mc/data/json.h>
#include <mc/data/alloc.h>
#include <mc/error.h>

typedef struct MC_UIModule MC_UIModule;
typedef struct MC_UIModuleDef MC_UIModuleDef;

typedef unsigned MC_UIModuleID;
typedef unsigned MC_UITypeID;

struct MC_UIModuleDef {
    char *name;
    MC_UIViewDef *views;
    char **types;
    MC_Error (*value_from_json)(MC_UITypeID type, const MC_Json *json, MC_Alloc *alloc, void **raw);
    MC_Error (*value_to_json)(MC_UITypeID type, const void *raw, MC_Alloc *alloc, MC_Json **json);
};

#endif // MC_UI_MODULE_H
