#include <mc/data/img/bmp.h>

#include <mc/util/error.h>

static MC_Error validate(const MC_BmpHdr *hdr, const MC_BmpDIBHdr *dib, const void *data);
static MC_Error validate_core(const MC_BmpHdr *hdr, const MC_BmpCorehdr *dib);
static MC_Error validate_info(const MC_BmpHdr *hdr, const MC_BmpInfohdr *dib, const void *data);

MC_Error mc_bmp_infohdr_init(MC_BmpHdr *hdr, MC_BmpInfohdr *dib,
    MC_BmpInfohdrCompression compression, uint32_t width, uint32_t height, uint16_t bpp)
{
    MC_RETURN_INVALID(width == 0 || height == 0);

    switch (compression){
    case MC_BMPI_COMP_RGB:
        MC_RETURN_INVALID(bpp % 3 != 0);
        break;
    case MC_BMPI_COMP_RLE8:
        MC_RETURN_INVALID(bpp != 8);
        return MCE_NOT_IMPLEMENTED;
    case MC_BMPI_COMP_RLE4:
        MC_RETURN_INVALID(bpp != 4);
        return MCE_NOT_IMPLEMENTED;
    case MC_BMPI_COMP_BITFIELDS:
    case MC_BMPI_COMP_JPEG:
    case MC_BMPI_COMP_PNG:
        return MCE_NOT_IMPLEMENTED;
    case MC_BMPI_COMP_ALPHABITFIELDS:
        MC_RETURN_INVALID(bpp % 4 != 0);
        break;
    case MC_BMPI_COMP_CMYK:
    case MC_BMPI_COMP_CMYKRLE4:
    case MC_BMPI_COMP_CMYKRLE8:
        return MCE_NOT_IMPLEMENTED;
    default: return MCE_INVALID_INPUT;
    }

    *hdr = (MC_BmpHdr){
        .magic = MC_BMP_INFOHDR,
        .size = width * height * bpp + MC_BMP_HDR_SIZE + MC_BMP_INFOHDR_SIZE,
        .data_offset = MC_BMP_HDR_SIZE + MC_BMP_INFOHDR_SIZE,
    };

    *dib = (MC_BmpInfohdr){
        .size = MC_BMP_INFOHDR_SIZE,
        .width = width,
        .height = height,
        .color_planes = 1,
        .bits_per_pixel = bpp,
        .compression = compression,
        .data_size = (width * height * bpp + 7) / 8,
        .pixel_per_meter_x = 5000,
        .pixel_per_meter_y = 5000,
        .colors_in_palette = 1 << (bpp - 1),
        .important_colors = 0,
    };

    return MCE_OK;
}

MC_Error mc_bmp_save(MC_Stream *stream, const MC_BmpHdr *hdr, const MC_BmpDIBHdr *dib, const void *data){
    MC_RETURN_ERROR(validate(hdr, dib, data));

    MC_RETURN_ERROR(mc_pack(stream, "<HIHHI",
        hdr->magic,
        hdr->size,
        hdr->reserved[0],
        hdr->reserved[1],
        hdr->data_offset));
    
    uint32_t data_size = 0;

    if(hdr->magic == MC_BMP_COREHDR && dib->info.size == MC_BMP_COREHDR_SIZE){
        MC_RETURN_ERROR(mc_pack(stream, "<IHHHH",
            dib->core.size,
            dib->core.width,
            dib->core.height,
            dib->core.color_planes,
            dib->core.bits_per_pixel));

        data_size = (dib->core.width * dib->core.height * dib->core.bits_per_pixel + 7) / 8;
    }
    else if(hdr->magic == MC_BMP_INFOHDR && dib->info.size == MC_BMP_INFOHDR_SIZE){
        MC_RETURN_ERROR(mc_pack(stream, "<IIIHHIIIIII",
            dib->info.size,
            dib->info.width,
            dib->info.height,
            dib->info.color_planes,
            dib->info.bits_per_pixel,
            dib->info.compression,
            dib->info.data_size,
            dib->info.pixel_per_meter_y,
            dib->info.pixel_per_meter_x,
            dib->info.colors_in_palette,
            dib->info.important_colors));

        data_size = dib->info.data_size;
    }

    MC_RETURN_INVALID(data_size == 0);

    return mc_write(stream, data_size, data, NULL);
}

static MC_Error validate(const MC_BmpHdr *hdr, const MC_BmpDIBHdr *dib, const void *data){
    MC_RETURN_INVALID(hdr == NULL);
    MC_RETURN_INVALID(dib == NULL);
    MC_RETURN_INVALID(data == NULL);

    switch (hdr->magic){
    case MC_BMP_HDR_TYPE('B', 'M'):
        if(dib->core.size == MC_BMP_COREHDR_SIZE) return validate_core(hdr, &dib->core);
        if(dib->info.size == MC_BMP_INFOHDR_SIZE) return validate_info(hdr, &dib->info, data);
        return MCE_INVALID_INPUT;
    case MC_BMP_HDR_TYPE('B', 'A'):return MCE_NOT_SUPPORTED;
    case MC_BMP_HDR_TYPE('C', 'I'): return MCE_NOT_SUPPORTED;
    case MC_BMP_HDR_TYPE('C', 'P'): return MCE_NOT_SUPPORTED;
    case MC_BMP_HDR_TYPE('I', 'C'): return MCE_NOT_SUPPORTED;
    case MC_BMP_HDR_TYPE('P', 'T'): return MCE_NOT_SUPPORTED;
    default: return MCE_INVALID_INPUT;
    }
}

static MC_Error validate_core(const MC_BmpHdr *hdr, const MC_BmpCorehdr *dib){
    MC_RETURN_INVALID(hdr->data_offset < MC_BMP_HDR_SIZE + MC_BMP_COREHDR_SIZE);
    MC_RETURN_INVALID(hdr->size == 0);
    MC_RETURN_INVALID(hdr->size < hdr->data_offset + (dib->width * dib->height * dib->bits_per_pixel + 7) / 8);

    return MCE_OK;
}

static MC_Error validate_info(const MC_BmpHdr *hdr, const MC_BmpInfohdr *dib, const void *data){
    (void)data;

    switch (dib->compression){
    case MC_BMPI_COMP_RGB:
        MC_RETURN_INVALID(dib->bits_per_pixel % 3 != 0);;
        break;
    case MC_BMPI_COMP_RLE8: return MCE_NOT_SUPPORTED;
    case MC_BMPI_COMP_RLE4: return MCE_NOT_SUPPORTED;
    case MC_BMPI_COMP_BITFIELDS: return MCE_NOT_SUPPORTED;
    case MC_BMPI_COMP_JPEG: return MCE_NOT_SUPPORTED;
    case MC_BMPI_COMP_PNG: return MCE_NOT_SUPPORTED;
    case MC_BMPI_COMP_ALPHABITFIELDS:
        MC_RETURN_INVALID(dib->bits_per_pixel % 4 != 0);
        break;
    case MC_BMPI_COMP_CMYK: return MCE_NOT_SUPPORTED;
    case MC_BMPI_COMP_CMYKRLE8: return MCE_NOT_SUPPORTED;
    case MC_BMPI_COMP_CMYKRLE4: return MCE_NOT_SUPPORTED;
    default: return MCE_INVALID_INPUT;
    }

    MC_RETURN_INVALID(hdr->size == 0);
    MC_RETURN_INVALID(hdr->data_offset < MC_BMP_HDR_SIZE + MC_BMP_INFOHDR_SIZE);
    MC_RETURN_INVALID(dib->data_size < (dib->width * dib->height * dib->bits_per_pixel + 7) / 8);
    MC_RETURN_INVALID(hdr->size < hdr->data_offset + dib->data_size);

    return MCE_OK;
}