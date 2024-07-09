#ifndef MC_WM_MOUSE_BUTTON_H
#define MC_WM_MOUSE_BUTTON_H

#define MC_ITER_MOUSE_BUTTONS() \
    MOUSE_BUTTON(UNKNOWN, "Unknown") \
    MOUSE_BUTTON(LEFT, "Left") \
    MOUSE_BUTTON(MIDDLE, "Wheel") \
    MOUSE_BUTTON(RIGHT, "Right") \
    MOUSE_BUTTON(BUTTON4, "Button 4") \
    MOUSE_BUTTON(BUTTON5, "Button 5") \
    MOUSE_BUTTON(BUTTON6, "Button 6") \
    MOUSE_BUTTON(BUTTON7, "Button 7") \
    MOUSE_BUTTON(BUTTON8, "Button 8") \
    MOUSE_BUTTON(BUTTON9, "Button 9") \
    MOUSE_BUTTON(BUTTON10, "Button 10") \
    MOUSE_BUTTON(BUTTON11, "Button 11") \
    MOUSE_BUTTON(BUTTON12, "Button 12") \
    MOUSE_BUTTON(BUTTON13, "Button 13") \
    MOUSE_BUTTON(BUTTON14, "Button 14") \
    MOUSE_BUTTON(BUTTON15, "Button 15") \
    MOUSE_BUTTON(BUTTON16, "Button 16") \

typedef unsigned MC_MouseButton;
enum MC_MouseButton{
    #define MOUSE_BUTTON(NAME, ...) MC_MOUSE_##NAME,
    MC_ITER_MOUSE_BUTTONS()
    #undef MOUSE_BUTTON
};

inline const char *mc_mouse_button_str(MC_MouseButton button){
    switch (button){
    #define MOUSE_BUTTON(NAME, HUMAN, ...) case MC_MOUSE_##NAME: return HUMAN;
    MC_ITER_MOUSE_BUTTONS()
    #undef MOUSE_BUTTON
    default: return (void*)0;
    }
}

inline const char *mc_mouse_button_enum_str(MC_MouseButton button){
    switch (button){
    #define MOUSE_BUTTON(NAME, ...) case MC_MOUSE_##NAME: return #NAME;
    MC_ITER_MOUSE_BUTTONS()
    #undef MOUSE_BUTTON
    default: return (void*)0;
    }
}

#endif // MC_WM_MOUSE_BUTTON_H
