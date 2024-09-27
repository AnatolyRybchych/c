#ifndef MC_UTIL_MEMORY_H
#define MC_UTIL_MEMORY_H

#include <mc/error.h>

#include <stdalign.h>
#include <stdint.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdarg.h>

#define MC_ALIGN(TO, VALUE) (((VALUE) + (TO) - 1) & ~((TO) - 1))

#endif // MC_UTIL_MEMORY_H
