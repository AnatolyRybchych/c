#ifndef MC_ASSERT_H
#define MC_ASSERT_H

#include <mc/error.h>
#include <mc/io/file.h>

#include <assert.h>
#include <stdlib.h>

#define MC_ASSERT_BUG(...) assert(__VA_ARGS__)
#define MC_ASSERT_FAULT(...) assert(__VA_ARGS__)

#define MC_REQUIRE(...) __mc_require(__VA_ARGS__, #__VA_ARGS__, __FILE__, __LINE__, __func__)

static inline MC_Error __mc_require(MC_Error ret, const char *expr, const char *file, int line, const char *func){
    if(ret != MCE_AGAIN && ret != MCE_OK){
        mc_fmt(MC_STDERR, "%s:%i %s() %s -> %s\n", file, line, func, expr, mc_strerror(ret));
        exit(1);
    }

    return ret;
}

#endif // MC_ASSERT_H
