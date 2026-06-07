#include <mc/win32_wm/graphics.h>

#include <mc/data/alloc.h>
#include <mc/util/error.h>

#include <stdint.h>
#include <malloc.h>

struct MC_TargetBuffer{
    HDC dc;
    HBITMAP bitmap;
    HBITMAP old_bitmap;
    void *bits;
    MC_Size2U size;
};

struct MC_TargetGraphics{
    HWND hwnd;
    HDC window_dc;
    HDC target_dc;
    MC_TargetBuffer *buffer;

    size_t tmp_size;
    void *tmp;
};

static void destroy(MC_TargetGraphics *g);

static MC_Error begin(MC_TargetGraphics *g);
static MC_Error end(MC_TargetGraphics *g);

static MC_Error init_buffer(MC_TargetGraphics *g, MC_TargetBuffer *buffer, MC_Size2U size_px);
static void destroy_buffer(MC_TargetGraphics *g, MC_TargetBuffer *buffer);
static MC_Error select_buffer(MC_TargetGraphics *g, MC_TargetBuffer *buffer);
static MC_Error write(MC_TargetGraphics *g, MC_Vec2i pos, MC_Size2U size, MC_TargetBuffer *src, MC_Vec2i src_pos);
static MC_Error write_pixels(MC_TargetGraphics *g, MC_Vec2i pos, MC_Size2U size, const MC_AColor pixels[size.height][size.width], MC_Vec2i src_pos);
static MC_Error read_pixels(MC_TargetGraphics *g, MC_Vec2i pos, MC_Size2U size, MC_AColor pixels[size.height][size.width]);

static MC_Error clear(MC_TargetGraphics *g, MC_Color color);
static MC_Error get_size(MC_TargetGraphics *g, MC_Size2U *size);

static MC_Error ensure_tmp(MC_TargetGraphics *g, size_t size);

const MC_GraphicsVtab vtab = {
    .buffer_ctx_size = sizeof(MC_TargetBuffer),
    .name = "WIN32 GDI",

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

MC_Error mc_win32_graphics_init(MC_Graphics **g, HWND hwnd){
    HDC window_dc = GetDC(hwnd);
    if(window_dc == NULL){
        return MCE_NOT_SUPPORTED;
    }

    MC_TargetGraphics target = {
        .hwnd = hwnd,
        .window_dc = window_dc,
        .target_dc = window_dc,
        .buffer = NULL,
        .tmp_size = 0,
        .tmp = NULL,
    };

    return mc_graphics_init(g, &vtab, sizeof(MC_TargetGraphics), &target);
}

static void destroy(MC_TargetGraphics *g){
    if(g->tmp){
        mc_free(NULL, g->tmp);
    }

    if(g->window_dc){
        ReleaseDC(g->hwnd, g->window_dc);
    }
}

static MC_Error begin(MC_TargetGraphics *g){
    (void)g;
    return MCE_OK;
}

static MC_Error end(MC_TargetGraphics *g){
    (void)g;
    GdiFlush();
    return MCE_OK;
}

static MC_Error init_buffer(MC_TargetGraphics *g, MC_TargetBuffer *buffer, MC_Size2U size_px){
    HDC dc = CreateCompatibleDC(g->window_dc);
    if(dc == NULL){
        return MCE_OUT_OF_MEMORY;
    }

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = (LONG)size_px.width;
    bmi.bmiHeader.biHeight = -(LONG)size_px.height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void *bits = NULL;
    HBITMAP bitmap = CreateDIBSection(g->window_dc, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    if(bitmap == NULL){
        DeleteDC(dc);
        return MCE_OUT_OF_MEMORY;
    }

    buffer->dc = dc;
    buffer->bitmap = bitmap;
    buffer->old_bitmap = SelectObject(dc, bitmap);
    buffer->bits = bits;
    buffer->size = size_px;

    return MCE_OK;
}

static void destroy_buffer(MC_TargetGraphics *g, MC_TargetBuffer *buffer){
    (void)g;

    if(buffer->dc){
        SelectObject(buffer->dc, buffer->old_bitmap);
        DeleteDC(buffer->dc);
    }

    if(buffer->bitmap){
        DeleteObject(buffer->bitmap);
    }
}

static MC_Error select_buffer(MC_TargetGraphics *g, MC_TargetBuffer *buffer){
    g->buffer = buffer;
    g->target_dc = buffer ? buffer->dc : g->window_dc;
    return MCE_OK;
}

static MC_Error write(MC_TargetGraphics *g, MC_Vec2i pos, MC_Size2U size, MC_TargetBuffer *src, MC_Vec2i src_pos){
    HDC src_dc = src ? src->dc : g->window_dc;

    BitBlt(g->target_dc, pos.x, pos.y, (int)size.width, (int)size.height,
        src_dc, src_pos.x, src_pos.y, SRCCOPY);

    return MCE_OK;
}

static MC_Error write_pixels(MC_TargetGraphics *g, MC_Vec2i pos, MC_Size2U size, const MC_AColor pixels[size.height][size.width], MC_Vec2i src_pos){
    (void)src_pos;

    MC_RETURN_ERROR(ensure_tmp(g, (size_t)size.width * size.height * 4));

    const MC_AColor (*px)[size.width] = (const void*)pixels;
    uint8_t (*bgra)[size.width][4] = g->tmp;

    for(unsigned y = 0; y < size.height; y++){
        for(unsigned x = 0; x < size.width; x++){
            bgra[y][x][0] = px[y][x].b;
            bgra[y][x][1] = px[y][x].g;
            bgra[y][x][2] = px[y][x].r;
            bgra[y][x][3] = px[y][x].a;
        }
    }

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = (LONG)size.width;
    bmi.bmiHeader.biHeight = (LONG)size.height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    SetDIBitsToDevice(g->target_dc, pos.x, pos.y, size.width, size.height,
        0, 0, 0, size.height, g->tmp, &bmi, DIB_RGB_COLORS);

    return MCE_OK;
}

static MC_Error read_pixels(MC_TargetGraphics *g, MC_Vec2i pos, MC_Size2U size, MC_AColor pixels[size.height][size.width]){
    HDC dc = CreateCompatibleDC(g->target_dc);
    if(dc == NULL){
        return MCE_OUT_OF_MEMORY;
    }

    HBITMAP bitmap = CreateCompatibleBitmap(g->window_dc, (int)size.width, (int)size.height);
    if(bitmap == NULL){
        DeleteDC(dc);
        return MCE_OUT_OF_MEMORY;
    }

    HBITMAP old_bitmap = SelectObject(dc, bitmap);
    BitBlt(dc, 0, 0, (int)size.width, (int)size.height, g->target_dc, pos.x, pos.y, SRCCOPY);

    MC_Error status = ensure_tmp(g, (size_t)size.width * size.height * 4);
    if(status != MCE_OK){
        SelectObject(dc, old_bitmap);
        DeleteObject(bitmap);
        DeleteDC(dc);
        return status;
    }

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = (LONG)size.width;
    bmi.bmiHeader.biHeight = (LONG)size.height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    GetDIBits(dc, bitmap, 0, size.height, g->tmp, &bmi, DIB_RGB_COLORS);

    const uint8_t (*bgra)[size.width][4] = g->tmp;
    for(unsigned y = 0; y < size.height; y++){
        for(unsigned x = 0; x < size.width; x++){
            pixels[y][x].b = bgra[y][x][0];
            pixels[y][x].g = bgra[y][x][1];
            pixels[y][x].r = bgra[y][x][2];
            pixels[y][x].a = bgra[y][x][3];
        }
    }

    SelectObject(dc, old_bitmap);
    DeleteObject(bitmap);
    DeleteDC(dc);

    return MCE_OK;
}

static MC_Error clear(MC_TargetGraphics *g, MC_Color color){
    MC_Size2U size;
    get_size(g, &size);

    RECT rect = {0, 0, (LONG)size.width, (LONG)size.height};
    HBRUSH brush = CreateSolidBrush(RGB(color.r, color.g, color.b));
    if(brush == NULL){
        return MCE_OUT_OF_MEMORY;
    }

    FillRect(g->target_dc, &rect, brush);
    DeleteObject(brush);

    return MCE_OK;
}

static MC_Error get_size(MC_TargetGraphics *g, MC_Size2U *size){
    if(g->buffer){
        *size = g->buffer->size;
        return MCE_OK;
    }

    RECT rect;
    if(!GetClientRect(g->hwnd, &rect)){
        return MCE_NOT_SUPPORTED;
    }

    size->width = (unsigned)(rect.right - rect.left);
    size->height = (unsigned)(rect.bottom - rect.top);

    return MCE_OK;
}

static MC_Error ensure_tmp(MC_TargetGraphics *g, size_t size){
    if(g->tmp_size >= size){
        return MCE_OK;
    }

    void *grown = realloc(g->tmp, size);
    if(grown == NULL){
        return MCE_OUT_OF_MEMORY;
    }

    g->tmp = grown;
    g->tmp_size = size;

    return MCE_OK;
}
