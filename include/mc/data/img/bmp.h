#ifndef MC_DATA_IMG_BMP_H
#define MC_DATA_IMG_BMP_H

#include <mc/error.h>
#include <mc/io/stream.h>

#include <stdint.h>

#define MC_BMP_HDR_TYPE(CH1, CH2) ((uint16_t)(CH1) | ((uint16_t)(CH2) << 8))

#define MC_BMP_HDR_SIZE 12
#define MC_BMP_COREHDR_SIZE 12
#define MC_BMP_INFOHDR_SIZE 40

#define MC_BMP_ITER_INFOHDR_COMPRESSION() \
    COMPRESSION(RGB,            0) \
    COMPRESSION(RLE8,           1) \
    COMPRESSION(RLE4,           2) \
    COMPRESSION(BITFIELDS,      3) \
    COMPRESSION(JPEG,           4) \
    COMPRESSION(PNG,            5) \
    COMPRESSION(ALPHABITFIELDS, 6) \
    COMPRESSION(CMYK,           11) \
    COMPRESSION(CMYKRLE8,       12) \
    COMPRESSION(CMYKRLE4,       13) \

typedef struct MC_BmpHdr MC_BmpHdr;
typedef union MC_BmpDIBHdr MC_BmpDIBHdr;

typedef uint16_t MC_BmpHdrType;
enum MC_BmpHdrType{
    MC_BMP_COREHDR = MC_BMP_HDR_TYPE('B', 'M'),
    MC_BMP_INFOHDR = MC_BMP_HDR_TYPE('B', 'M'),
};

typedef uint32_t MC_BmpInfohdrCompression;
enum MC_BmpInfohdrCompression{
    #define COMPRESSION(NAME, VALUE, ...) MC_BMPI_COMP_##NAME = (VALUE),
    MC_BMP_ITER_INFOHDR_COMPRESSION()
    #undef COMPRESSION
};

typedef struct MC_BmpCorehdr MC_BmpCorehdr;
typedef struct MC_BmpInfohdr MC_BmpInfohdr;


// TODO: fix ALPHABITFIELDS
MC_Error mc_bmp_infohdr_init(MC_BmpHdr *hdr, MC_BmpInfohdr *dib,
    MC_BmpInfohdrCompression compression, uint32_t width, uint32_t height, uint16_t bpp);

// pixels should be converted to the LSB by the callee
MC_Error mc_bmp_save(MC_Stream *stream, const MC_BmpHdr *hdr, const MC_BmpDIBHdr *dib, const void *pixels);

struct MC_BmpHdr{
    MC_BmpHdrType magic;
    uint32_t size;
    uint16_t reserved[2];
    uint32_t data_offset;
};

struct MC_BmpCorehdr{
    uint32_t size;
    uint16_t width;
    uint16_t height;
    uint16_t color_planes;
    uint16_t bits_per_pixel;
};

struct MC_BmpInfohdr{
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint16_t color_planes;
    uint16_t bits_per_pixel;
    MC_BmpInfohdrCompression compression;
    uint32_t data_size;
    uint32_t pixel_per_meter_y;
    uint32_t pixel_per_meter_x;
    uint32_t colors_in_palette;
    uint32_t important_colors;
};

union MC_BmpDIBHdr{
    MC_BmpCorehdr core;
    MC_BmpInfohdr info;
};

#endif // MC_DATA_IMG_BMP_H
