#ifndef MC_UI_MODULE_H
#define MC_UI_MODULE_H

#include <mc/ui/view.h>

typedef struct MC_UIModule MC_UIModule;
typedef struct MC_UIModuleDef MC_UIModuleDef;

struct MC_UIModuleDef {
    char *name;
    MC_UIViewDef (*views)[];
};

#endif // MC_UI_MODULE_H
