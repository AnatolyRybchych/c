#ifndef MC_UTIL_MEMORY_H
#define MC_UTIL_MEMORY_H

#include <mc/error.h>

#include <stdalign.h>
#include <stdint.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdarg.h>

#define MC_ALIGN(TO, VALUE) (((VALUE) + (TO) - 1) & ~((TO) - 1))
#define MC_U16(B1, B2) (((B1)<<8) | (B2))
#define MC_U32(B1, B2, B3, B4) ((MC_U16(B1, B2) << 16) | MC_U16(B3, B4))
#define MC_U64(B1, B2, B3, B4, B5, B6, B7, B8) ((MC_U32(B1, B2, B3, B4) << 32) | MC_U32(B5, B6, B7, B8))

#endif // MC_UTIL_MEMORY_H
