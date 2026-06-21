#include <mc/util/assert.h>
#include <mc/time.h>
#include <mc/os/file.h>

#include <mc/wm/wm.h>
#include <mc/wm/event.h>
#include <mc/wm/key.h>

#ifdef _WIN32
#include <mc/win32_wm/wm.h>
#define DEMO_WM_VTAB mc_win32_wm_vtab
#else
#include <mc/xlib_wm/wm.h>
#define DEMO_WM_VTAB mc_xlib_wm_vtab
#endif

#include <mc/graphics/graphics.h>
#include <mc/ui/ui.h>

#include <stdbool.h>

#define SQUARE 160

static uint8_t ramp(unsigned v){
    return (uint8_t)(v * 255 / (SQUARE - 1));
}

static MC_AColor color(unsigned frame, unsigned x, unsigned y){
    unsigned phase = frame % 512;
    return (MC_AColor){
        .r = ramp(x),
        .g = ramp(y),
        .b = (uint8_t)(phase < 256 ? phase : 511 - phase),
        .a = 255,
    };
}

static void draw(MC_Graphics *g, unsigned frame){
    MC_Size2U size;
    MC_REQUIRE(mc_graphics_get_size(g, &size));

    MC_REQUIRE(mc_graphics_begin(g));
    MC_REQUIRE(mc_graphics_clear(g, (MC_Color){.r = 24, .g = 24, .b = 32}));

    MC_AColor pixels[SQUARE][SQUARE];
    for(unsigned y = 0; y < SQUARE; y++){
        for(unsigned x = 0; x < SQUARE; x++){
            pixels[y][x] = color(frame, x, y);
        }
    }

    MC_Vec2i pos = {
        .x = size.width  > SQUARE ? (int)(size.width  - SQUARE) / 2 : 0,
        .y = size.height > SQUARE ? (int)(size.height - SQUARE) / 2 : 0,
    };

    MC_REQUIRE(mc_graphics_write_pixels(g, pos,
        (MC_Size2U){.width = SQUARE, .height = SQUARE}, (void*)pixels, (MC_Vec2i){0, 0}));

    MC_REQUIRE(mc_graphics_end(g));
}

int main(void){
    MC_WM *wm;
    MC_REQUIRE(mc_wm_init(&wm, DEMO_WM_VTAB));

    MC_UI *ui;
    MC_REQUIRE(mc_ui_init(NULL, &ui));
    MC_REQUIRE(mc_ui_register_module(ui, &mc_ui_module_def));

    MC_UIElementID element;
    MC_REQUIRE(mc_ui_create_element(ui, mc_ui_find_view(ui, "View"), &element));
    MC_REQUIRE(mc_ui_element_destroy(ui, element));

    MC_WMWindow *window;
    MC_REQUIRE(mc_wm_window_init(mc_wm_get_ref(wm), &window));

    MC_WindowRef *ref = mc_wm_window_get_ref(window);
    MC_REQUIRE(mc_wm_window_set_title(ref, MC_STRC("mc gui demo - type text, Ctrl+V paste; F/M/N/R state; Q/Esc quit")));
    MC_REQUIRE(mc_wm_window_set_size(ref, MC_WM_AREA_WINDOW, (MC_Size2U){.width = 640, .height = 480}));

    MC_Graphics *g;
    MC_REQUIRE(mc_wm_window_get_graphic(window, &g));

    bool running = true;
    unsigned frame = 0;
    while(running){
        MC_WMEvent event;
        while(mc_wm_poll_event(wm, &event) == MCE_OK){
            switch(event.type){
            case MC_WME_WINDOW_CLOSE_REQUESTED:
                running = false;
                break;
            case MC_WME_FOCUS_GAINED:
                mc_fmt(MC_STDERR, "focus gained\n");
                break;
            case MC_WME_FOCUS_LOST:
                mc_fmt(MC_STDERR, "focus lost\n");
                break;
            case MC_WME_WINDOW_STATE_CHANGED:
                mc_fmt(MC_STDERR, "state changed: %u (cached %u)\n",
                    (unsigned)event.as.window_state_changed.state,
                    (unsigned)mc_wm_window_cached_get_state(window));
                break;
            case MC_WME_TEXT_INPUT:
                mc_fmt(MC_STDERR, "text: %s\n", event.as.text_input.utf8);
                break;
            case MC_WME_PASTE_TEXT:
                mc_fmt(MC_STDERR, "paste: %.*s\n",
                    (int)mc_str_len(event.as.paste_text.text), event.as.paste_text.text.beg);
                break;
            case MC_WME_KEY_DOWN:
                switch(event.as.key_down.key){
                case MC_KEY_ESCAPE:
                case MC_KEY_Q: running = false;                                          break;
                case MC_KEY_F: mc_wm_window_set_state(ref, MC_WM_WINDOW_STATE_FULLSCREEN); break;
                case MC_KEY_M: mc_wm_window_set_state(ref, MC_WM_WINDOW_STATE_MAXIMIZED);  break;
                case MC_KEY_N: mc_wm_window_set_state(ref, MC_WM_WINDOW_STATE_MINIMIZED);  break;
                case MC_KEY_R: mc_wm_window_set_state(ref, MC_WM_WINDOW_STATE_NORMAL);     break;
                default: break;
                }
                break;
            default:
                break;
            }
        }

        draw(g, frame++);
        mc_sleep(&(MC_Time){.nsec = 16000000});
    }

    mc_wm_window_destroy(window);
    mc_ui_destroy(ui);
    mc_wm_destroy(wm);
    return 0;
}
