#include <mc/graphics/graphics.h>
#include <mc/graphics/target.h>
#include <mc/data/img/bmp.h>

#include <malloc.h>
#include <stdalign.h>
#include <stdint.h>
#include <memory.h>

struct MC_Graphics{
    MC_GraphicsVtab vtab;
    MC_TargetGraphics *target;

    size_t tmp_size;
    void *tmp;

    alignas(void*) uint8_t data[];
};

struct MC_GBuffer{
    MC_Graphics *g;
    MC_TargetBuffer *target;
    alignas(void*) uint8_t data[];
};

extern inline uint8_t mc_u8_clamp(int val);

MC_Error mc_graphics_init(MC_Graphics **ret_g, const MC_GraphicsVtab *vtab, size_t ctx_size, const void *ctx){
    MC_Graphics *g = malloc(sizeof(MC_Graphics) + ctx_size);
    if(g == NULL){
        return MCE_OUT_OF_MEMORY;
    }

    if(ctx != NULL){
        memcpy(g->data, ctx, ctx_size);
    }

    g->vtab = *vtab;
    g->target = (MC_TargetGraphics*)g->data;
    g->tmp = NULL;
    g->tmp_size = 0;

    *ret_g = g;
    return MCE_OK;
}

void mc_graphics_destroy(MC_Graphics *g){
    g->vtab.destroy(g->target);
    free(g);
}

MC_Error mc_graphics_begin(MC_Graphics *g){
    if(g->vtab.begin == NULL){
        return MCE_OK;
    }

    return g->vtab.begin(g->target);
}

MC_Error mc_graphics_end(MC_Graphics *g){
    if(g->vtab.end == NULL){
        return MCE_OK;
    }

    return g->vtab.end(g->target);
}

MC_Error mc_graphics_create_buffer(MC_Graphics *g, MC_GBuffer **ret_buffer, MC_Size2U size_px){
    *ret_buffer = NULL;

    if(g->vtab.init_buffer == NULL){
        return MCE_NOT_SUPPORTED;
    }

    MC_GBuffer *buffer = malloc(sizeof(MC_GBuffer) + g->vtab.buffer_ctx_size);
    if(buffer == NULL){
        return MCE_OUT_OF_MEMORY;
    }

    memset(buffer, 0, sizeof(buffer) + g->vtab.buffer_ctx_size);
    buffer->target = (MC_TargetBuffer*)buffer->data;
    buffer->g = g;

    MC_Error status = g->vtab.init_buffer(g->target, buffer->target, size_px);
    if(status != MCE_OK){
        free(buffer);
    }
    else{
        *ret_buffer = buffer;
    }

    return status;
}

void mc_graphics_destroy_buffer(MC_GBuffer *buffer){
    if(buffer == NULL){
        return;
    }

    if(buffer->g->vtab.destroy_buffer){
        buffer->g->vtab.destroy_buffer(buffer->g->target, buffer->target);
    }

    free(buffer);
}

MC_Error mc_graphics_select_buffer(MC_Graphics *g, MC_GBuffer *buffer){
    if(buffer){
        if(!g->vtab.select_buffer){
            return buffer ? MCE_NOT_SUPPORTED : MCE_OK;
        }

        if(buffer->g != g){
            return MCE_INVALID_INPUT;
        }

        return g->vtab.select_buffer(g->target, buffer->target);
    }

    return g->vtab.select_buffer
        ? g->vtab.select_buffer(g->target, NULL) 
        :MCE_OK;
}

MC_Error mc_graphics_write(MC_Graphics *g, MC_Point2I pos, MC_Size2U size, MC_GBuffer *buffer, MC_Point2I src_pos){
    if(buffer){
        if(!g->vtab.write){
            return buffer ? MCE_NOT_SUPPORTED : MCE_OK;
        }

        if(buffer->g != g){
            return MCE_INVALID_INPUT;
        }

        return g->vtab.write(g->target, pos, size, buffer->target, src_pos);
    }

    return g->vtab.write ? g->vtab.write(g->target, pos, size, NULL, src_pos)  :MCE_OK;
}

MC_Error mc_graphics_write_pixels(MC_Graphics *g, MC_Point2I pos, MC_Size2U size, const MC_AColor pixels[size.height][size.width], MC_Point2I src_pos){
    if(g->vtab.write_pixels == NULL){
        return MCE_NOT_SUPPORTED;
    }

    return g->vtab.write_pixels(g->target, pos, size, pixels, src_pos);
}

MC_Error mc_graphics_read_pixels(MC_Graphics *g, MC_Point2I pos, MC_Size2U size, MC_AColor pixels[size.height][size.width]){
    if(g->vtab.read_pixels == NULL){
        return MCE_NOT_SUPPORTED;
    }

    return g->vtab.read_pixels(g->target, pos, size, pixels);
}

MC_Error mc_graphics_get_size(MC_Graphics *g, MC_Size2U *size){
    if(g->vtab.get_size == NULL){
        return MCE_NOT_SUPPORTED;
    }

    return g->vtab.get_size(g->target, size);
}

struct MC_TargetGraphics *mc_graphics_target(MC_Graphics *g){
    return g->target;
}

MC_Error mc_graphics_clear(MC_Graphics *g, MC_Color color){
    if(g->vtab.clear){
        return g->vtab.clear(g->target, color);
    }

    return MCE_NOT_SUPPORTED;
}

MC_Error mc_graphics_dump(MC_Graphics *g, struct MC_Stream *stream){
    if(g->vtab.get_size == NULL || g->vtab.read_pixels == NULL){
        return MCE_NOT_SUPPORTED;
    }

    MC_Size2U size;
    MC_RETURN_ERROR(g->vtab.get_size(g->target, &size));

    size_t payload_size = size.width * size.height * sizeof(MC_AColor);

    if(g->tmp_size < payload_size){
        void *tmp = realloc(g->tmp, payload_size);
        if(tmp == NULL){
            return MCE_OUT_OF_MEMORY;
        }

        g->tmp_size = payload_size;
        g->tmp = tmp;
    }

    MC_AColor (*pixels)[size.height][size.width] = g->tmp;
    uint8_t (*payload)[size.height][size.width][3] = g->tmp;

    MC_RETURN_ERROR(g->vtab.read_pixels(g->target, (MC_Point2I){0}, size, (void*)pixels));
    
    for(size_t y = 0; y < size.height; y++){
        for(size_t x = 0; x < size.width; x++){
            MC_Color c = (*pixels)[y][x].color;
            // float a = (*pixels)[y][x].alpha;
            (*payload)[y][x][0] = mc_u8_clamp(c.b * 255.0);
            (*payload)[y][x][1] = mc_u8_clamp(c.g * 255.0);
            (*payload)[y][x][2] = mc_u8_clamp(c.r * 255.0);
            // (*payload)[y][x][3] = mc_u8_clamp(a * 255.0);
        }
    }

    MC_BmpHdr hdr;
    MC_BmpDIBHdr dib;
    //TODO: fix alpha
    MC_RETURN_ERROR(mc_bmp_infohdr_init(&hdr, &dib.info, MC_BMPI_COMP_RGB, size.width, size.height, 24));

    MC_RETURN_ERROR(mc_bmp_save(stream, &hdr, &dib, pixels));

    return MCE_OK;
}
