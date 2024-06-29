#ifndef MC_DATA_STRUCT_H
#define MC_DATA_STRUCT_H

#include <mc/error.h>

#include <stddef.h>
#include <stdarg.h>

int mc_struct_calcsize(const char *fmt);
int mc_struct_vnpack(void *buffer, unsigned buffer_size, const char *fmt, va_list args);
int mc_struct_npack(void *buffer, unsigned buffer_size, const char *fmt, ...);
int mc_struct_pack(void *buffer, const char *fmt, ...);

#endif // MC_DATA_STRUCT_H
