#ifndef MC_WM_KEY_H
#define MC_WM_KEY_H

#define MC_ITER_KEYS() \
    MC_KEY(BACKSPACE) \
    MC_KEY(TAB) \
    MC_KEY(LINEFEED) \
    MC_KEY(CLEAR) \
    MC_KEY(RETURN) \
    MC_KEY(PAUSE) \
    MC_KEY(SCROLL_LOCK) \
    MC_KEY(SYS_REQ) \
    MC_KEY(ESCAPE) \
    MC_KEY(DELETE) \
    \
    MC_KEY(HOME) \
    MC_KEY(LEFT) \
    MC_KEY(UP) \
    MC_KEY(RIGHT) \
    MC_KEY(DOWN) \
    MC_KEY(PRIOR) \
    MC_KEY(PAGE_UP) \
    MC_KEY(NEXT) \
    MC_KEY(PAGE_DOWN) \
    MC_KEY(END) \
    MC_KEY(BEGIN) \
    \
    MC_KEY(SELECT) \
    MC_KEY(PRINT) \
    MC_KEY(EXECUTE) \
    MC_KEY(INSERT) \
    MC_KEY(UNDO) \
    MC_KEY(REDO) \
    MC_KEY(MENU) \
    MC_KEY(FIND) \
    MC_KEY(CANCEL) \
    MC_KEY(HELP) \
    MC_KEY(BREAK) \
    MC_KEY(MODE_SWITCH) \
    MC_KEY(NUM_LOCK) \
    \
    MC_KEY(KP_SPACE) \
    MC_KEY(KP_TAB) \
    MC_KEY(KP_ENTER) \
    MC_KEY(KP_F1) \
    MC_KEY(KP_F2) \
    MC_KEY(KP_F3) \
    MC_KEY(KP_F4) \
    MC_KEY(KP_HOME) \
    MC_KEY(KP_LEFT) \
    MC_KEY(KP_UP) \
    MC_KEY(KP_RIGHT) \
    MC_KEY(KP_DOWN) \
    MC_KEY(KP_PRIOR) \
    MC_KEY(KP_NEXT) \
    MC_KEY(KP_END) \
    MC_KEY(KP_BEGIN) \
    MC_KEY(KP_INSERT) \
    MC_KEY(KP_DELETE) \
    MC_KEY(KP_EQUAL) \
    MC_KEY(KP_MULTIPLY) \
    MC_KEY(KP_ADD) \
    MC_KEY(KP_SEPARATOR) \
    MC_KEY(KP_SUBSTRACT) \
    MC_KEY(KP_DECIMAL) \
    MC_KEY(KP_DIVIDE) \
    MC_KEY(KP_0) \
    MC_KEY(KP_1) \
    MC_KEY(KP_2) \
    MC_KEY(KP_3) \
    MC_KEY(KP_4) \
    MC_KEY(KP_5) \
    MC_KEY(KP_6) \
    MC_KEY(KP_7) \
    MC_KEY(KP_8) \
    MC_KEY(KP_9) \
    \
    MC_KEY(F1) \
    MC_KEY(F2) \
    MC_KEY(F3) \
    MC_KEY(F4) \
    MC_KEY(F5) \
    MC_KEY(F6) \
    MC_KEY(F7) \
    MC_KEY(F8) \
    MC_KEY(F9) \
    MC_KEY(F10) \
    MC_KEY(F11) \
    MC_KEY(F12) \
    MC_KEY(F13) \
    MC_KEY(F14) \
    MC_KEY(F15) \
    MC_KEY(F16) \
    MC_KEY(F17) \
    MC_KEY(F18) \
    MC_KEY(F19) \
    MC_KEY(F20) \
    MC_KEY(F21) \
    MC_KEY(F22) \
    MC_KEY(F23) \
    MC_KEY(F24) \
    MC_KEY(F25) \
    MC_KEY(F26) \
    MC_KEY(F27) \
    MC_KEY(F28) \
    MC_KEY(F29) \
    MC_KEY(F30) \
    MC_KEY(F31) \
    MC_KEY(F32) \
    MC_KEY(F33) \
    MC_KEY(F34) \
    MC_KEY(F35) \
    \
    MC_KEY(SHIFT_L) \
    MC_KEY(SHIFT_R) \
    MC_KEY(CONTROL_L) \
    MC_KEY(CONTROL_R) \
    MC_KEY(CAPSLOCK) \
    MC_KEY(SHIFTLOCK) \
    MC_KEY(META_L) \
    MC_KEY(META_R) \
    MC_KEY(ALT_L) \
    MC_KEY(ALT_R) \
    MC_KEY(SUPER_L) \
    MC_KEY(SUPER_R) \
    MC_KEY(HYPER_L) \
    MC_KEY(HYPER_R) \
    \
    MC_KEY(SPACE) \
    MC_KEY(EXCLAMATION) \
    MC_KEY(QUOTE) \
    MC_KEY(SIGN) \
    MC_KEY(DOLLAR) \
    MC_KEY(PERCENT) \
    MC_KEY(AMPERSAND) \
    MC_KEY(APOSTROPHE) \
    MC_KEY(PAREN_OPEN) \
    MC_KEY(PAREN_CLOSE) \
    MC_KEY(ASTERISK) \
    MC_KEY(PLUS) \
    MC_KEY(COMMA) \
    MC_KEY(MINUS) \
    MC_KEY(PERIOD) \
    MC_KEY(SLASH) \
    MC_KEY(0) \
    MC_KEY(1) \
    MC_KEY(2) \
    MC_KEY(3) \
    MC_KEY(4) \
    MC_KEY(5) \
    MC_KEY(6) \
    MC_KEY(7) \
    MC_KEY(8) \
    MC_KEY(9) \
    MC_KEY(COLON) \
    MC_KEY(SEMICOLON) \
    MC_KEY(LESS) \
    MC_KEY(EQUAL) \
    MC_KEY(GREATER) \
    MC_KEY(QUESTION) \
    MC_KEY(AT) \
    MC_KEY(A) \
    MC_KEY(B) \
    MC_KEY(C) \
    MC_KEY(D) \
    MC_KEY(E) \
    MC_KEY(F) \
    MC_KEY(G) \
    MC_KEY(H) \
    MC_KEY(I) \
    MC_KEY(J) \
    MC_KEY(K) \
    MC_KEY(L) \
    MC_KEY(M) \
    MC_KEY(N) \
    MC_KEY(O) \
    MC_KEY(P) \
    MC_KEY(Q) \
    MC_KEY(R) \
    MC_KEY(S) \
    MC_KEY(T) \
    MC_KEY(U) \
    MC_KEY(V) \
    MC_KEY(W) \
    MC_KEY(X) \
    MC_KEY(Y) \
    MC_KEY(Z) \
    MC_KEY(BRACKET_OPEN) \
    MC_KEY(BACKSLASH) \
    MC_KEY(BRACKET_CLOSE) \
    MC_KEY(ASCIICIRCUM) \
    MC_KEY(UNDERSCORE) \
    MC_KEY(GRAVE) \
    MC_KEY(BRACE_OPEN) \
    MC_KEY(BAR) \
    MC_KEY(BRACE_CLOSE) \
    MC_KEY(ASCIITILDE) \
    MC_KEY(NOBREAKSPACE) \
    MC_KEY(EXCLAMATIONDOWN) \

typedef unsigned MC_Key;
enum MC_Key{
    MC_KEY_UNKNOWN = 0,
    #define MC_KEY(NAME, ...) MC_KEY_##NAME,
    MC_ITER_KEYS()
    #undef MC_KEY
};

inline const char *mc_key_str(MC_Key key){
    switch (key){
    case MC_KEY_UNKNOWN: return "UNKNOWN";
    #define MC_KEY(NAME, ...) case MC_KEY_##NAME: return #NAME;
    MC_ITER_KEYS()
    #undef MC_KEY
    default: return mc_key_str(MC_KEY_UNKNOWN);
    }
}

#endif // MC_WM_KEY_H
