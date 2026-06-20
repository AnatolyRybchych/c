
#include <mc/ui/ui.h>

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
    }
};
