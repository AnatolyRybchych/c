#include <mc/data/str.h>

#include <string.h>

extern inline size_t mc_str_len(MC_Str str);
extern inline bool mc_str_starts_with(MC_Str str, MC_Str substr);
extern inline bool mc_str_empty(MC_Str str);
extern inline MC_Str mc_str_ltrim(MC_Str str);
extern inline MC_Str mc_str_rtrim(MC_Str str);
extern inline MC_Str mc_str_trim(MC_Str str);
extern inline MC_Str mc_str_toull(MC_Str str, uint64_t *val);
