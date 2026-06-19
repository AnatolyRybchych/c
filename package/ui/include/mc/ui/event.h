#ifndef MC_UI_EVENT_H
#define MC_UI_EVENT_H

#include <mc/error.h>

typedef struct MC_UiEvent MC_UiEvent;
typedef unsigned MC_UiEventType;

struct MC_UiEvent {
    MC_UiEventType type;
};

#endif // MC_UI_EVENT_H
