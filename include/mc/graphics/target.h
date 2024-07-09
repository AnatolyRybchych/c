#ifndef MC_GRAPHICS_TARGET_H
#define MC_GRAPHICS_TARGET_H

#include <mc/graphics/graphics.h>

typedef struct MC_TargetGraphics MC_TargetGraphics;
typedef struct MC_TargetBuffer MC_TargetBuffer;

struct MC_GraphicsVtab{
    char name[32];
    // size_t buffer_ctx_size;

    void (*destroy)(MC_TargetGraphics *g);

    MC_Error (*begin)(MC_TargetGraphics *g);
    MC_Error (*end)(MC_TargetGraphics *g);

    // MC_Error (*init_buffer)(MC_TargetGraphics *g, MC_TargetBuffer *buffer, MC_Size2U size_px);
    // void (*destroy_buffer)(MC_TargetGraphics *g, MC_TargetBuffer *buffer);
    // void (*swap)(MC_TargetGraphics *g, MC_TargetBuffer *buffer);

    MC_Error (*clear)(MC_TargetGraphics *g, MC_Color color);
    // MC_Error (*get_size_px)(MC_TargetGraphics *g, MC_Size2U *size);
};

#endif // MC_GRAPHICS_TARGET_H
