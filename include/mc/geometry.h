#ifndef MC_GEOMETRY_H
#define MC_GEOMETRY_H

typedef struct MC_Point2I MC_Point2I;
typedef struct MC_Point2F MC_Point2F;

typedef struct MC_Size2U MC_Size2U;
typedef struct MC_Size2F MC_Size2F;

typedef struct MC_Rect2IU MC_Rect2IU;
typedef struct MC_Rect2FF MC_Rect2FF;

struct MC_Point2I{
    int x, y;
};

struct MC_Point2F{
    float x, y;
};

struct MC_Size2U{
    unsigned width, height;
};

struct MC_Size2F{
    float widht, height;
};

struct MC_Rect2IU{
    int x, y;
    unsigned width, height;
};

struct MC_Rect2FF{
    float x, y;
    float width, height;
};


#endif // MC_GEOMETRY_H
