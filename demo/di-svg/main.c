#include <mc/util/assert.h>
#include <mc/util/error.h>
#include <mc/time.h>
#include <mc/os/file.h>

#include <mc/wm/wm.h>
#include <mc/wm/event.h>
#include <mc/wm/key.h>
#include <mc/xlib_wm/wm.h>

#include <mc/graphics/graphics.h>
#include <mc/graphics/di.h>

#include <stdbool.h>

#define SHAPE_SIZE 256
#define CELLS 2
#define STROKE 0.02f

static const MC_AColor INK  = {.r = 0x25, .g = 0x63, .b = 0xEB, .a = 0xFF};
static const MC_AColor DARK = {.r = 0x16, .g = 0x3B, .b = 0x8D, .a = 0xFF};
static const MC_AColor PAGE = {.r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0xFF};

static const MC_Vec2f g_beg0 = {0.66977f, 0.32769f};
static const MC_SemiBezier4f g_seg0[] = {
    {.c1 = {0.66977f, 0.38267f}, .c2 = {0.65331f, 0.42424f}, .p2 = {0.62039f, 0.45240f}},
    {.c1 = {0.58747f, 0.48057f}, .c2 = {0.54013f, 0.49466f}, .p2 = {0.47837f, 0.49466f}},
    {.c1 = {0.45054f, 0.49466f}, .c2 = {0.42475f, 0.49211f}, .p2 = {0.40099f, 0.48702f}},
    {.c1 = {0.39030f, 0.50925f}, .c2 = {0.37961f, 0.53148f}, .p2 = {0.36892f, 0.55370f}},
    {.c1 = {0.36994f, 0.55947f}, .c2 = {0.37656f, 0.56490f}, .p2 = {0.38877f, 0.56999f}},
    {.c1 = {0.40099f, 0.57508f}, .c2 = {0.41626f, 0.57763f}, .p2 = {0.43459f, 0.57763f}},
    {.c1 = {0.48176f, 0.57763f}, .c2 = {0.52893f, 0.57763f}, .p2 = {0.57610f, 0.57763f}},
    {.c1 = {0.62768f, 0.57763f}, .c2 = {0.66595f, 0.58883f}, .p2 = {0.69089f, 0.61123f}},
    {.c1 = {0.71583f, 0.63362f}, .c2 = {0.72831f, 0.66451f}, .p2 = {0.72831f, 0.70387f}},
    {.c1 = {0.72831f, 0.73950f}, .c2 = {0.71838f, 0.77056f}, .p2 = {0.69853f, 0.79703f}},
    {.c1 = {0.67867f, 0.82350f}, .c2 = {0.64957f, 0.84394f}, .p2 = {0.61123f, 0.85837f}},
    {.c1 = {0.57288f, 0.87279f}, .c2 = {0.52639f, 0.88000f}, .p2 = {0.47175f, 0.88000f}},
    {.c1 = {0.40659f, 0.88000f}, .c2 = {0.35696f, 0.86999f}, .p2 = {0.32285f, 0.84997f}},
    {.c1 = {0.28875f, 0.82994f}, .c2 = {0.27169f, 0.80144f}, .p2 = {0.27169f, 0.76445f}},
    {.c1 = {0.27169f, 0.74646f}, .c2 = {0.27780f, 0.72873f}, .p2 = {0.29002f, 0.71125f}},
    {.c1 = {0.30224f, 0.69378f}, .c2 = {0.32463f, 0.67333f}, .p2 = {0.35721f, 0.64991f}},
    {.c1 = {0.33787f, 0.64347f}, .c2 = {0.32158f, 0.63244f}, .p2 = {0.30835f, 0.61683f}},
    {.c1 = {0.29511f, 0.60121f}, .c2 = {0.28849f, 0.58442f}, .p2 = {0.28849f, 0.56643f}},
    {.c1 = {0.31530f, 0.53623f}, .c2 = {0.34211f, 0.50602f}, .p2 = {0.36892f, 0.47582f}},
    {.c1 = {0.31530f, 0.45071f}, .c2 = {0.28849f, 0.40133f}, .p2 = {0.28849f, 0.32769f}},
    {.c1 = {0.28849f, 0.27543f}, .c2 = {0.30504f, 0.23504f}, .p2 = {0.33812f, 0.20654f}},
    {.c1 = {0.37121f, 0.17803f}, .c2 = {0.41932f, 0.16378f}, .p2 = {0.48244f, 0.16378f}},
    {.c1 = {0.49499f, 0.16378f}, .c2 = {0.51111f, 0.16505f}, .p2 = {0.53080f, 0.16760f}},
    {.c1 = {0.55048f, 0.17014f}, .c2 = {0.56558f, 0.17311f}, .p2 = {0.57610f, 0.17650f}},
    {.c1 = {0.61360f, 0.15767f}, .c2 = {0.65110f, 0.13883f}, .p2 = {0.68860f, 0.12000f}},
    {.c1 = {0.69454f, 0.12730f}, .c2 = {0.70048f, 0.13459f}, .p2 = {0.70642f, 0.14189f}},
    {.c1 = {0.68283f, 0.16632f}, .c2 = {0.65925f, 0.19076f}, .p2 = {0.63566f, 0.21519f}},
    {.c1 = {0.65840f, 0.24064f}, .c2 = {0.66977f, 0.27814f}, .p2 = {0.66977f, 0.32769f}},
};

static const MC_Vec2f g_beg1 = {0.64889f, 0.71965f};
static const MC_SemiBezier4f g_seg1[] = {
    {.c1 = {0.64889f, 0.70031f}, .c2 = {0.64296f, 0.68521f}, .p2 = {0.63108f, 0.67435f}},
    {.c1 = {0.61920f, 0.66349f}, .c2 = {0.60121f, 0.65806f}, .p2 = {0.57712f, 0.65806f}},
    {.c1 = {0.51536f, 0.65806f}, .c2 = {0.45359f, 0.65806f}, .p2 = {0.39183f, 0.65806f}},
    {.c1 = {0.37758f, 0.67027f}, .c2 = {0.36595f, 0.68580f}, .p2 = {0.35696f, 0.70463f}},
    {.c1 = {0.34797f, 0.72347f}, .c2 = {0.34347f, 0.74103f}, .p2 = {0.34347f, 0.75732f}},
    {.c1 = {0.34347f, 0.78651f}, .c2 = {0.35399f, 0.80746f}, .p2 = {0.37503f, 0.82019f}},
    {.c1 = {0.39607f, 0.83291f}, .c2 = {0.42831f, 0.83928f}, .p2 = {0.47175f, 0.83928f}},
    {.c1 = {0.52842f, 0.83928f}, .c2 = {0.57211f, 0.82876f}, .p2 = {0.60283f, 0.80772f}},
    {.c1 = {0.63354f, 0.78668f}, .c2 = {0.64889f, 0.75732f}, .p2 = {0.64889f, 0.71965f}},
};

static const MC_Vec2f g_beg2 = {0.47938f, 0.45597f};
static const MC_SemiBezier4f g_seg2[] = {
    {.c1 = {0.51637f, 0.45597f}, .c2 = {0.54259f, 0.44536f}, .p2 = {0.55803f, 0.42415f}},
    {.c1 = {0.57347f, 0.40294f}, .c2 = {0.58119f, 0.37079f}, .p2 = {0.58119f, 0.32769f}},
    {.c1 = {0.58119f, 0.28255f}, .c2 = {0.57322f, 0.25040f}, .p2 = {0.55727f, 0.23123f}},
    {.c1 = {0.54132f, 0.21205f}, .c2 = {0.51570f, 0.20246f}, .p2 = {0.48040f, 0.20246f}},
    {.c1 = {0.44477f, 0.20246f}, .c2 = {0.41864f, 0.21214f}, .p2 = {0.40201f, 0.23148f}},
    {.c1 = {0.38538f, 0.25082f}, .c2 = {0.37707f, 0.28289f}, .p2 = {0.37707f, 0.32769f}},
    {.c1 = {0.37707f, 0.37248f}, .c2 = {0.38521f, 0.40506f}, .p2 = {0.40150f, 0.42543f}},
    {.c1 = {0.41779f, 0.44579f}, .c2 = {0.44375f, 0.45597f}, .p2 = {0.47938f, 0.45597f}},
};

#define G_CONTOURS 3
static const MC_DiContour g_contours[G_CONTOURS] = {
    {.beg = g_beg0, .n = sizeof(g_seg0) / sizeof(*g_seg0), .segments = g_seg0},
    {.beg = g_beg1, .n = sizeof(g_seg1) / sizeof(*g_seg1), .segments = g_seg1},
    {.beg = g_beg2, .n = sizeof(g_seg2) / sizeof(*g_seg2), .segments = g_seg2},
};

static MC_Error build_outline(MC_Di *di, MC_DiShape *shape){
    for(size_t i = 0; i < G_CONTOURS; i++){
        const MC_DiContour *c = &g_contours[i];
        MC_RETURN_ERROR(mc_di_shape_contour(di, shape, c->beg, c->n, c->segments, STROKE));
    }
    return MCE_OK;
}

static MC_Error build_fill(MC_Di *di, MC_DiShape *shape){
    return mc_di_shape_region_contours(di, shape, G_CONTOURS, g_contours);
}

static void flip_vertical(MC_DiBuffer *buffer){
    MC_Size2U size = mc_di_size(buffer);
    MC_AColor *px = mc_di_pixels(buffer);

    for(unsigned y = 0; y < size.height / 2; y++){
        MC_AColor *top = px + (size_t)y * size.width;
        MC_AColor *bottom = px + (size_t)(size.height - 1 - y) * size.width;
        for(unsigned x = 0; x < size.width; x++){
            MC_AColor tmp = top[x];
            top[x] = bottom[x];
            bottom[x] = tmp;
        }
    }
}

static MC_Error build_cells(MC_Di *di, MC_DiBuffer *buffers[CELLS]){
    MC_Size2U size = {.width = SHAPE_SIZE, .height = SHAPE_SIZE};
    MC_Rect2IU cell = {.x = 0, .y = 0, .width = SHAPE_SIZE, .height = SHAPE_SIZE};

    MC_DiShape *outline, *fill;
    MC_RETURN_ERROR(mc_di_shape_create(di, &outline, size));
    MC_RETURN_ERROR(mc_di_shape_create(di, &fill, size));
    MC_RETURN_ERROR(build_outline(di, outline));
    MC_RETURN_ERROR(build_fill(di, fill));

    for(int i = 0; i < CELLS; i++){
        MC_RETURN_ERROR(mc_di_create(di, &buffers[i], size));
        mc_di_clear(di, buffers[i], PAGE);
    }

    MC_RETURN_ERROR(mc_di_fill_shape(di, buffers[0], outline, cell, INK));

    MC_RETURN_ERROR(mc_di_fill_shape(di, buffers[1], fill, cell, INK));
    MC_RETURN_ERROR(mc_di_fill_shape(di, buffers[1], outline, cell, DARK));

    for(int i = 0; i < CELLS; i++){
        flip_vertical(buffers[i]);
    }

    mc_di_shape_delete(di, outline);
    mc_di_shape_delete(di, fill);
    return MCE_OK;
}

static void draw(MC_Graphics *g, MC_DiBuffer *buffers[CELLS]){
    MC_Size2U size;
    MC_REQUIRE(mc_graphics_get_size(g, &size));

    unsigned s = SHAPE_SIZE;
    int x0 = (int)(size.width - s * CELLS) / 2;
    int y0 = (int)(size.height - s) / 2;

    MC_REQUIRE(mc_graphics_begin(g));
    MC_REQUIRE(mc_graphics_clear(g, mc_color(PAGE)));
    for(int i = 0; i < CELLS; i++){
        MC_REQUIRE(mc_graphics_write_pixels(g, (MC_Vec2i){.x = x0 + i * (int)s, .y = y0},
            (MC_Size2U){.width = s, .height = s}, (void*)mc_di_pixels(buffers[i]), (MC_Vec2i){0, 0}));
    }
    MC_REQUIRE(mc_graphics_end(g));
}

int main(void){
    MC_WM *wm;
    MC_REQUIRE(mc_wm_init(&wm, mc_xlib_wm_vtab));

    MC_WMWindow *window;
    MC_REQUIRE(mc_wm_window_init(wm, &window));
    MC_REQUIRE(mc_wm_window_set_title(window, MC_STRC("di glyph: contour outline | region fill - Q/Esc quit")));
    MC_REQUIRE(mc_wm_window_set_size(window, (MC_Size2U){.width = 640, .height = 320}));

    MC_Graphics *g;
    MC_REQUIRE(mc_wm_window_get_graphic(window, &g));

    MC_Di *di;
    MC_REQUIRE(mc_di_init(&di));

    MC_DiBuffer *buffers[CELLS] = {0};
    MC_REQUIRE(build_cells(di, buffers));

    bool running = true;
    bool dirty = true;
    while(running){
        MC_WMEvent event;
        while(mc_wm_poll_event(wm, &event) == MCE_OK){
            switch(event.type){
            case MC_WME_WINDOW_CLOSE_REQUESTED:
                running = false;
                break;
            case MC_WME_WINDOW_REDRAW_REQUESTED:
            case MC_WME_WINDOW_RESIZED:
                dirty = true;
                break;
            case MC_WME_KEY_DOWN:
                switch(event.as.key_down.key){
                case MC_KEY_ESCAPE:
                case MC_KEY_Q: running = false; break;
                default: break;
                }
                break;
            default:
                break;
            }
        }

        if(dirty){
            draw(g, buffers);
            dirty = false;
        }
        mc_sleep(&(MC_Time){.nsec = 16000000});
    }

    for(int i = 0; i < CELLS; i++){
        mc_di_delete(di, buffers[i]);
    }
    mc_di_destroy(di);
    mc_wm_window_destroy(window);
    mc_wm_destroy(wm);
    return 0;
}
