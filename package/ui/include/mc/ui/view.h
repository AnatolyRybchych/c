#ifndef MC_UI_VIEW_H
#define MC_UI_VIEW_H

#include <mc/ui/event.h>

#include <mc/data/json.h>
#include <mc/error.h>

#include <stddef.h>

typedef struct MC_UIView MC_UIView;
typedef struct MC_UIViewDef MC_UIViewDef;

typedef unsigned MC_UIViewID;
typedef unsigned MC_UIProp;

struct MC_UIViewDef {
    char *name;
    char *parent;

    char *(*props)[];
    char *(*events)[];

    void (*ctor)(MC_UIView *view);
    void (*dtor)(MC_UIView *view);
    void (*handle_event)(MC_UIView *view, const MC_UiEvent *event);

    MC_Error (*prop_to_json)(const MC_UIView *view, MC_UIProp prop, MC_Alloc *alloc, MC_Json **json);
    MC_Error (*prop_from_json)(MC_UIView *view, MC_UIProp prop, const MC_Json *json);
    MC_Error (*event_to_json)(const MC_UiEvent *event, MC_Alloc *alloc, MC_Json **json);
    MC_Error (*event_from_json)(const MC_Json *json, MC_UIProp prop, MC_UiEvent **event);
};

#endif // MC_UI_VIEW_H
