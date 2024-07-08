#ifndef MC_GEOMETRY_H
#define MC_GEOMETRY_H

typedef struct MC_Point2I MC_Point2I;
typedef struct MC_Size2U MC_Size2U;
typedef struct MC_Rect2IU MC_Rect2IU;

struct MC_Point2I{
    int x, y;
};

struct MC_Size2U{
    unsigned width, height;
};

struct MC_Rect2IU{
    int x, y;
    unsigned width, height;
};


#endif // MC_GEOMETRY_H
