#ifndef MC_UI_VIEW_H
#define MC_UI_VIEW_H

#include <mc/ui/event.h>

#include <mc/data/json.h>
#include <mc/error.h>

#include <stddef.h>

typedef struct MC_UIViewDef MC_UIViewDef;
typedef struct MC_UIElement MC_UIElement;

typedef unsigned MC_UIViewID;
typedef unsigned MC_UIProp;

#define MC_UI_VIEW_INVALID ((MC_UIViewID)-1)

struct MC_UIViewDef {
    char *name;
    char *parent;
    size_t size;

    char **props;
    char **events;

    void (*ctor)(MC_UIElement *element);
    void (*dtor)(MC_UIElement *element);
    void (*handle_event)(MC_UIElement *element, const MC_UiEvent *event);

    MC_Error (*prop_to_json)(const MC_UIElement *element, MC_UIProp prop, MC_Alloc *alloc, MC_Json **json);
    MC_Error (*prop_from_json)(MC_UIElement *element, MC_UIProp prop, const MC_Json *json);
    MC_Error (*event_to_json)(const MC_UiEvent *event, MC_Alloc *alloc, MC_Json **json);
    MC_Error (*event_from_json)(const MC_Json *json, MC_UIProp prop, MC_UiEvent **event);
};

#endif // MC_UI_VIEW_H
