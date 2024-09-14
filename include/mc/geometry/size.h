#ifndef MC_GEOMETRY_SIZE_H
#define MC_GEOMETRY_SIZE_H

#define MC_SISE2U(WIDTH, HEIGHT) (MC_Size2U){.width = WIDTH, .height = HEIGHT}
#define MC_SISE_EQ(SZ1, SZ2) ((SZ1).width == (SZ2).width && (SZ1).height == (SZ2).height)

typedef struct MC_Size2U MC_Size2U;
typedef struct MC_Size2F MC_Size2F;

struct MC_Size2U{
    unsigned width, height;
};

struct MC_Size2F{
    float widht, height;
};

#endif // MC_GEOMETRY_SIZE_H
