#ifndef MC_DL_H
#define MC_DL_H

#include <mc/error.h>
#include <mc/data/str.h>

typedef struct MC_DLib MC_DLib;

MC_Error mc_dlib_open(MC_DLib **lib, MC_Str path);
void mc_dlib_close(MC_DLib *lib);

void *mc_dlib_get(MC_DLib *lib, MC_Str symbol);
MC_Str mc_dlib_path(const MC_DLib *lib);

#endif // MC_DL_H
