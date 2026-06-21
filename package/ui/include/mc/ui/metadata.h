#ifndef MC_UI_METADATA_H
#define MC_UI_METADATA_H

#include <mc/ui/module.h>
#include <mc/ui/view.h>
#include <mc/ui/event.h>

#include <mc/data/vector.h>
#include <mc/data/string.h>
#include <mc/data/json.h>
#include <mc/data/alloc.h>
#include <mc/error.h>

#include <stddef.h>

typedef struct MC_UIInfo MC_UIInfo;
typedef struct MC_UIModuleInfo MC_UIModuleInfo;
typedef struct MC_UIViewInfo MC_UIViewInfo;
typedef struct MC_UIEventInfo MC_UIEventInfo;
typedef struct MC_UIPropInfo MC_UIPropInfo;
typedef struct MC_UITypeInfo MC_UITypeInfo;

struct MC_UIModuleInfo {
    MC_String *name;
    MC_UIModuleID id;
    unsigned prop_base;
    unsigned event_base;
    unsigned type_base;
    unsigned views_cnt;
    unsigned props_cnt;
    unsigned events_cnt;
    unsigned types_cnt;
    MC_Error (*value_from_json)(MC_UITypeID type, const MC_Json *json, MC_Alloc *alloc, void **raw);
    MC_Error (*value_to_json)(MC_UITypeID type, const void *raw, MC_Alloc *alloc, MC_Json **json);
};

struct MC_UIViewInfo {
    MC_String *name;
    MC_UIViewID id;
    MC_UIViewID parent;
    size_t size;
    unsigned prop_base;
    unsigned event_base;
    unsigned props_cnt;
    unsigned events_cnt;
    void (*ctor)(MC_UIElement *element);
    void (*dtor)(MC_UIElement *element);
    void (*handle_event)(MC_UIElement *element, const MC_UiEvent *event);
    MC_Error (*prop_to_json)(const MC_UIElement *element, MC_UIProp prop, MC_Alloc *alloc, MC_Json **json);
    MC_Error (*prop_from_json)(MC_UIElement *element, MC_UIProp prop, const MC_Json *json);
    MC_Error (*event_to_json)(const MC_UiEvent *event, MC_Alloc *alloc, MC_Json **json);
    MC_Error (*event_from_json)(const MC_Json *json, MC_UIProp prop, MC_UiEvent **event);
};

struct MC_UIEventInfo {
    unsigned id;
    MC_String *type;
};

struct MC_UIPropInfo {
    unsigned id;
    MC_String *name;
};

struct MC_UITypeInfo {
    unsigned id;
    MC_String *name;
};

MC_DEFINE_VECTOR(ModuleInfoList, MC_UIModuleInfo);
MC_DEFINE_VECTOR(ViewInfoList, MC_UIViewInfo);
MC_DEFINE_VECTOR(EventInfoList, MC_UIEventInfo);
MC_DEFINE_VECTOR(PropInfoList, MC_UIPropInfo);
MC_DEFINE_VECTOR(TypeInfoList, MC_UITypeInfo);

struct MC_UIInfo {
    ModuleInfoList *modules;
    ViewInfoList *views;
    PropInfoList *props;
    EventInfoList *events;
    TypeInfoList *types;
};

MC_Error mc_ui_metadata_to_json(const MC_UIInfo *metadata, MC_Alloc *alloc, MC_Json **out);

#endif // MC_UI_METADATA_H
