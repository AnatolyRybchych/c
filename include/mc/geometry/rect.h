#ifndef MC_GEONETRY_RECT_H
#define MC_GEONETRY_RECT_H

typedef struct MC_Rect2IU MC_Rect2IU;
typedef struct MC_Rect2FF MC_Rect2FF;

struct MC_Rect2IU{
    int x, y;
    unsigned width, height;
};

struct MC_Rect2FF{
    float x, y;
    float width, height;
};

#endif // MC_GEONETRY_RECT_H
