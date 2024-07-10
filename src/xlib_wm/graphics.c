#include <mc/xlib_wm/graphics.h>

#include <stdint.h>

struct MC_TargetGraphics{
    Drawable drawable;
    Display *dpy;
    GC gc;
};

static void destroy(MC_TargetGraphics *g);

static MC_Error begin(MC_TargetGraphics *g);
static MC_Error end(MC_TargetGraphics *g);

static MC_Error clear(MC_TargetGraphics *g, MC_Color color);

static MC_Size2U get_size(MC_TargetGraphics *g);
static uint32_t get_color(MC_Color color);

const MC_GraphicsVtab vtab = {
    .name = "X11 GC",

    .destroy = destroy,

    .begin = begin,
    .end = end,
    .clear = clear
};

MC_Error mc_xlib_graphics_init(MC_Graphics **g, Display *dpy, Drawable drawable){
    MC_TargetGraphics target = {
        .drawable = drawable,
        .dpy = dpy,
        .gc = XCreateGC(dpy, drawable, 0, NULL)
    };

    return mc_graphics_init(g, &vtab, sizeof(MC_TargetGraphics), &target);
}

static void destroy(MC_TargetGraphics *g){
    XFreeGC(g->dpy, g->gc);
}

static MC_Error begin(MC_TargetGraphics *g){
    XSync(g->dpy, False);
    return MCE_OK;
}

static MC_Error end(MC_TargetGraphics *g){
    XSync(g->dpy, False);
    XFlush(g->dpy);
    return MCE_OK;
}

static MC_Error clear(MC_TargetGraphics *g, MC_Color color){
    MC_Size2U size = get_size(g);
    (void)size;

    XSetForeground(g->dpy, g->gc, get_color(color));
    
    XFillRectangle(g->dpy, g->drawable, g->gc, 0, 0, size.width, size.height);

    return MCE_OK;
}

static MC_Size2U get_size(MC_TargetGraphics *g){
    MC_Size2U size;

    XGetGeometry(g->dpy, g->drawable, &(Window){0}, &(int){0}, &(int){0},
        &size.width, &size.height, &(unsigned int){0}, &(unsigned int){0});

    return size;
}

static uint32_t get_color(MC_Color color){
    return ((uint32_t)(color.b * 255) << 0)
         | ((uint32_t)(color.g * 255) << 8)
         | ((uint32_t)(color.r * 255) << 16);
}
