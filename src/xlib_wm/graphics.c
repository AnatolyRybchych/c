#include <mc/xlib_wm/graphics.h>

#include <X11/Xutil.h>

#include <stdint.h>
#include <malloc.h>

struct MC_TargetGraphics{
    size_t tmp_size;
    void *tmp;

    int screen;
    int depth;
    Visual *visual;

    MC_TargetBuffer *buffer;
    Drawable drawing_target;
    Drawable drawable;
    Display *dpy;
    GC gc;
};

struct MC_TargetBuffer{
    Pixmap pixmap;
};

static void destroy(MC_TargetGraphics *g);

static MC_Error begin(MC_TargetGraphics *g);
static MC_Error end(MC_TargetGraphics *g);

static MC_Error init_buffer(MC_TargetGraphics *g, MC_TargetBuffer *buffer, MC_Size2U size_px);
static void destroy_buffer(MC_TargetGraphics *g, MC_TargetBuffer *buffer);
static MC_Error select_buffer(MC_TargetGraphics *g, MC_TargetBuffer *buffer);
static MC_Error write(MC_TargetGraphics *g, MC_Point2I pos, MC_Size2U size, MC_TargetBuffer *src, MC_Point2I src_pos);
static MC_Error write_pixels(MC_TargetGraphics *g, MC_Point2I pos, MC_Size2U size, const MC_AColor pixels[size.height][size.width], MC_Point2I src_pos);
static MC_Error read_pixels(MC_TargetGraphics *g, MC_Point2I pos, MC_Size2U size, MC_AColor pixels[size.height][size.width]);

static MC_Error clear(MC_TargetGraphics *g, MC_Color color);

static MC_Error get_size(MC_TargetGraphics *g, MC_Size2U *size);

static uint32_t get_color(MC_Color color);

const MC_GraphicsVtab vtab = {
    .buffer_ctx_size = sizeof(MC_TargetBuffer),
    .name = "X11 GC",

    .destroy = destroy,

    .begin = begin,
    .end = end,

    .init_buffer = init_buffer,
    .destroy_buffer = destroy_buffer,
    .select_buffer = select_buffer,
    .write = write,
    .write_pixels = write_pixels,
    .read_pixels = read_pixels,

    .clear = clear,
    .get_size = get_size,
};

MC_Error mc_xlib_graphics_init(MC_Graphics **g, Display *dpy, Drawable drawable){
    int screen = DefaultScreen(dpy);
    int depth = DefaultDepth(dpy, screen);

    MC_TargetGraphics target = {
        .tmp_size = 0,
        .tmp = NULL,

        .screen = screen,
        .depth = depth,
        .visual = DefaultVisual(dpy, screen),

        .buffer = NULL,
        .drawing_target = drawable,
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

static MC_Error init_buffer(MC_TargetGraphics *g, MC_TargetBuffer *buffer, MC_Size2U size_px){
    buffer->pixmap = XCreatePixmap(g->dpy, g->drawable, size_px.width, size_px.height, g->depth);
    return MCE_OK;
}

static void destroy_buffer(MC_TargetGraphics *g, MC_TargetBuffer *buffer){
    if(g->tmp){
        free(g->tmp);
    }

    XFreePixmap(g->dpy, buffer->pixmap);
}

static MC_Error select_buffer(MC_TargetGraphics *g, MC_TargetBuffer *buffer){
    g->buffer = buffer;
    g->drawing_target = g->buffer ? g->buffer->pixmap : g->drawable;
    return MCE_OK;
}

static MC_Error write(MC_TargetGraphics *g, MC_Point2I pos, MC_Size2U size, MC_TargetBuffer *src, MC_Point2I src_pos){
    Drawable src_drawable = src ? src->pixmap : g->drawable;

    XCopyArea(g->dpy, g->drawing_target, src_drawable, g->gc,
        src_pos.x, src_pos.y, size.width, size.height, pos.x, pos.y);

    return MCE_OK;
}

static MC_Error write_pixels(MC_TargetGraphics *g, MC_Point2I pos, MC_Size2U size, const MC_AColor pixels[size.height][size.width], MC_Point2I src_pos){
    size_t pixel_data_size = size.width * size.height * 4;
    if(g->tmp_size < pixel_data_size){
        uint8_t *pixel_data = realloc(g->tmp, pixel_data_size);
        if(pixel_data == NULL){
            return MCE_OUT_OF_MEMORY;
        }

        g->tmp = pixel_data;
    }

    MC_AColor (*px)[size.height][size.width] = (void*)pixels;
    uint8_t (*pixel_data)[size.height][size.width][4] = g->tmp;

    for(size_t y = 0; y < size.height; y++){
        for(size_t x = 0; x < size.width; x++){
            (*pixel_data)[y][x][0] = mc_u8_clamp((*px)[size.height - y - 1][x].color.b * 255);
            (*pixel_data)[y][x][1] = mc_u8_clamp((*px)[size.height - y - 1][x].color.g * 255);
            (*pixel_data)[y][x][2] = mc_u8_clamp((*px)[size.height - y - 1][x].color.r * 255);
            (*pixel_data)[y][x][3] = mc_u8_clamp((*px)[size.height - y - 1][x].alpha * 255);
        }
    }

    XImage *image = XCreateImage(g->dpy, g->visual, g->depth, ZPixmap, 0, (char*)pixel_data, size.width, size.height, 32, 0);
    if(image == NULL){
        return MCE_OUT_OF_MEMORY;
    }

    XPutImage(g->dpy, g->drawing_target, g->gc, image, src_pos.x, src_pos.y, pos.x, pos.y, size.width, size.height);

    image->data = NULL;
    XDestroyImage(image);

    return MCE_OK;
}

static MC_Error read_pixels(MC_TargetGraphics *g, MC_Point2I pos, MC_Size2U size, MC_AColor pixels[size.height][size.width]){
    XImage *img = XGetImage(g->dpy, g->drawing_target, pos.x, pos.y, size.width, size.height, AllPlanes, ZPixmap);
    if(img == NULL){
        return MCE_OUT_OF_MEMORY;
    }

    uint8_t (*img_data)[size.height][size.width][4] = (void*)img->data;
    for (size_t y = 0; y < size.height; y++){
        for (size_t x = 0; x < size.width; x++){
            pixels[size.height - y - 1][x].color.b = (*img_data)[y][x][0] / 255.0;
            pixels[size.height - y - 1][x].color.g = (*img_data)[y][x][1] / 255.0;
            pixels[size.height - y - 1][x].color.r = (*img_data)[y][x][2] / 255.0;
            pixels[size.height - y - 1][x].alpha = (*img_data)[y][x][3] / 255.0;
        }
    }

    XDestroyImage(img);
    return MCE_OK;
}

static MC_Error clear(MC_TargetGraphics *g, MC_Color color){
    MC_Size2U size;
    get_size(g, &size);

    XSetForeground(g->dpy, g->gc, get_color(color));
    XFillRectangle(g->dpy, g->drawing_target, g->gc, 0, 0, size.width, size.height);

    return MCE_OK;
}

static MC_Error get_size(MC_TargetGraphics *g, MC_Size2U *size){
    XGetGeometry(g->dpy, g->drawing_target, &(Window){0}, &(int){0}, &(int){0},
        &size->width, &size->height, &(unsigned int){0}, &(unsigned int){0});

    return MCE_OK;
}

static uint32_t get_color(MC_Color color){
    return ((uint32_t)(color.b * 255) << 0)
         | ((uint32_t)(color.g * 255) << 8)
         | ((uint32_t)(color.r * 255) << 16);
}
