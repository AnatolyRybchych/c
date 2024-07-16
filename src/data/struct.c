#include <mc/data/struct.h>
#include <mc/util/assert.h>
#include <mc/util/memory.h>
#include <mc/util/minmax.h>

#include <stdint.h>
#include <stdalign.h>
#include <stdbool.h>
#include <float.h>
#include <ctype.h>
#include <memory.h>

#define U8(PTR) ((uint8_t*)(PTR))

#define GET_INT(ARGS) va_arg(ARGS, int)
#define GET_UINT(ARGS) va_arg(ARGS, unsigned)
#define GET_DOUBLE(ARGS) va_arg(ARGS, double)
#define GET_LONG(ARGS) va_arg(ARGS, long)
#define GET_LONG_LONG(ARGS) va_arg(ARGS, long long)
#define GET_ULONG_LONG(ARGS) va_arg(ARGS, unsigned long long)
#define GET_ZERO(ARGS) (0)
#define GET_BOOL(ARGS) (GET_INT(ARGS) ? 1 : 0)
#define GET_PTR(ARGS) va_arg(ARGS, void*)
#define GET_SIZE(ARGS) va_arg(ARGS, size_t)
#define GET_NULL(ARGS) (NULL)

#define ITER_BASIC_FMT() \
    FMT('x', uint8_t,   uint8_t,                GET_ZERO,       GET_NULL) \
    FMT('c', uint8_t,   char,                   GET_INT,        GET_PTR) \
    FMT('b', uint8_t,   signed char,            GET_INT,        GET_PTR) \
    FMT('B', int8_t,    unsigned char,          GET_INT,        GET_PTR) \
    FMT('?', uint8_t,   bool,                   GET_BOOL,       GET_PTR) \
    FMT('h', int16_t,   short,                  GET_INT,        GET_PTR) \
    FMT('H', uint16_t,  unsigned short,         GET_INT,        GET_PTR) \
    FMT('e', uint16_t,  uint16_t,               GET_DOUBLE,     GET_PTR) \
    FMT('i', int32_t,   int,                    GET_INT,        GET_PTR) \
    FMT('I', uint32_t,  unsigned int,           GET_UINT,       GET_PTR) \
    FMT('l', int32_t,   long int,               GET_LONG,       GET_PTR) \
    FMT('L', uint32_t,  unsigned long int,      GET_LONG,       GET_PTR) \
    FMT('f', float,     float,                  GET_DOUBLE,     GET_PTR) \
    FMT('q', int64_t,   long long int,          GET_LONG_LONG,  GET_PTR) \
    FMT('Q', uint64_t,  unsigned long long int, GET_ULONG_LONG, GET_PTR) \
    FMT('d', double,    double,                 GET_DOUBLE,     GET_PTR) \

#define ITER_ARR_FMT() \
    FMT('s', uint8_t,   char,   GET_INT,    GET_PTR) \
    FMT('p', uint8_t,   char,   GET_INT,    GET_PTR) 

#define ITER_NATIVE_FMT() \
    FMT('n',,           size_t, GET_SIZE,   GET_PTR) \
    FMT('P',,           void*,  GET_PTR,    GET_PTR) \

#define ITER_FMT() \
    ITER_BASIC_FMT() \
    ITER_ARR_FMT() \
    ITER_NATIVE_FMT()

static int calcsize_aligned_native(const char *fmt);
static int calcsize_unaligned(const char *fmt);
static int calcsize_unaligned_native(const char *fmt);

static int pack(void *buffer, unsigned buffer_size, const char *fmt, va_list args, bool align, bool inverse);
static int unpack(void *buffer, unsigned buffer_size, const char *fmt, va_list args, bool align, bool inverse);
static void inverse_endian(void *data, size_t size);

int mc_struct_calcsize(const char *fmt){
    switch (*fmt){
    case '@': return calcsize_aligned_native(fmt + 1);
    case '=': return calcsize_unaligned_native(fmt + 1);
    case '<': return calcsize_unaligned(fmt + 1);
    case '>': return calcsize_unaligned(fmt + 1);
    case '!': return calcsize_unaligned(fmt + 1);
    default : return calcsize_aligned_native(fmt);
    }
}

int mc_struct_vnpack(void *buffer, unsigned buffer_size, const char *fmt, va_list args){
    switch (*fmt){
    case '@': return pack(buffer, buffer_size, fmt + 1, args, true, false);
    case '=': return pack(buffer, buffer_size, fmt + 1, args, false, false);
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    case '<': return pack(buffer, buffer_size, fmt + 1, args, false, false);
    case '>': return pack(buffer, buffer_size, fmt + 1, args, false, true);
    case '!': return pack(buffer, buffer_size, fmt + 1, args, false, true);
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    case '<': return pack(buffer, buffer_size, fmt + 1, args, false, true);
    case '>': return pack(buffer, buffer_size, fmt + 1, args, false, false);
    case '!': return pack(buffer, buffer_size, fmt + 1, args, false, false);
#else
    static_assert(false, "unpack is not implemented for this byte order");
#endif
    default : return pack(buffer, buffer_size, fmt, args, true, false);
    }
}

int mc_struct_npack(void *buffer, unsigned buffer_size, const char *fmt, ...){
    va_list args;
    va_start(args, fmt);
    int res = mc_struct_vnpack(buffer, buffer_size, fmt, args);
    va_end(args);
    return res;
}

int mc_struct_pack(void *buffer, const char *fmt, ...){
    va_list args;
    va_start(args, fmt);
    int res = mc_struct_vnpack(buffer, ~(unsigned)0, fmt, args);
    va_end(args);
    return res;
}

int mc_struct_vnunpack(void *buffer, unsigned buffer_size, const char *fmt, va_list args){
    switch (*fmt){
    case '@': return unpack(buffer, buffer_size, fmt + 1, args, true, false);
    case '=': return unpack(buffer, buffer_size, fmt + 1, args, false, false);
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    case '<': return unpack(buffer, buffer_size, fmt + 1, args, false, false);
    case '>': return unpack(buffer, buffer_size, fmt + 1, args, false, true);
    case '!': return unpack(buffer, buffer_size, fmt + 1, args, false, true);
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    case '<': return unpack(buffer, buffer_size, fmt + 1, args, false, true);
    case '>': return unpack(buffer, buffer_size, fmt + 1, args, false, false);
    case '!': return unpack(buffer, buffer_size, fmt + 1, args, false, false);
#else
    static_assert(false, "unpack is not implemented for this byte order");
#endif
    default : return unpack(buffer, buffer_size, fmt, args, true, false);
    }
}

int mc_struct_nunpack(void *buffer, unsigned buffer_size, const char *fmt, ...){
    va_list args;
    va_start(args, fmt);
    int res = mc_struct_vnunpack(buffer, buffer_size, fmt, args);
    va_end(args);
    return res;
}

int mc_struct_unpack(void *buffer, const char *fmt, ...){
    va_list args;
    va_start(args, fmt);
    int res = mc_struct_vnunpack(buffer, ~(unsigned)0, fmt, args);
    va_end(args);
    return res;
}


static int calcsize_aligned_native(const char *fmt){
    int size = 0;
    int repeat = 0;
    int mul = 1;

    for(char ch = *fmt; ch; ch = *++fmt){
        if(isdigit(ch)){
            repeat *= 10;
            repeat += ch - '0';
            continue;
        }

        mul = repeat ? repeat : 1;
        repeat = 0;

        switch (ch){
        #define FMT(CH, _, NATIVE_TYPE, ...) \
        case CH: \
            size = MC_ALIGN(alignof(NATIVE_TYPE), size) + alignof(NATIVE_TYPE) * (mul -1) + sizeof(NATIVE_TYPE); \
            break;
        ITER_FMT()
        #undef FMT
        default: return -1;
        }
    }

    return repeat ? -1 : size;
}

static int calcsize_unaligned(const char *fmt){
    int size = 0;
    int repeat = 0;
    int mul = 1;

    for(char ch = *fmt; ch; ch = *++fmt){
        if(isdigit(ch)){
            repeat *= 10;
            repeat += ch - '0';
            continue;
        }

        mul = repeat ? repeat : 1;
        repeat = 0;

        switch (ch){
        #define FMT(CH, BASIC_TYPE, ...) \
        case CH: \
            size += sizeof(BASIC_TYPE) * mul; \
            break;
        ITER_BASIC_FMT()
        ITER_ARR_FMT()
        #undef FMT
        default: return -1;
        }
    }

    return repeat ? -1 : size;
}

static int calcsize_unaligned_native(const char *fmt){
    int size = 0;
    int repeat = 0;
    int mul = 1;

    for(char ch = *fmt; ch; ch = *++fmt){
        if(isdigit(ch)){
            repeat *= 10;
            repeat += ch - '0';
            continue;
        }

        mul = repeat ? repeat : 1;
        repeat = 0;

        switch (ch){
        #define FMT(CH, _, NATIVE_TYPE, ...) \
        case CH: size += sizeof(NATIVE_TYPE) * mul; \
        break;
        ITER_FMT()
        #undef FMT
        default: return -1;
        }
    }

    return repeat ? -1 : size;
}

static int pack(void *buffer, unsigned buffer_size, const char *fmt, va_list args, bool align, bool inverse){
    int size = calcsize_aligned_native(fmt);
    if(size < 0 || (unsigned)size > buffer_size){
        return size;
    }

    int cur_size = 0;
    int repeat = 0;
    int mul = 1;

    for(char ch = *fmt; ch; ch = *++fmt){
        if(isdigit(ch)){
            repeat *= 10;
            repeat += ch - '0';
            continue;
        }

        mul = repeat ? repeat : 1;
        repeat = 0;

        switch (ch){
        #define FMT(CH, _, NATIVE_TYPE, VA_GET, ...) \
        case CH:{ \
            for(int i = 0; i < mul; i++){ \
                if(align) cur_size = MC_ALIGN(alignof(NATIVE_TYPE), cur_size); \
                NATIVE_TYPE val = VA_GET(args); \
                memcpy(&U8(buffer)[cur_size], &val, sizeof(NATIVE_TYPE)); \
                if(inverse) inverse_endian(&U8(buffer)[cur_size], sizeof(NATIVE_TYPE)); \
                cur_size += sizeof(NATIVE_TYPE); \
            } \
        } break;
        ITER_BASIC_FMT()
        ITER_NATIVE_FMT()
        #undef FMT

        case 's':{ \
            const char *val = va_arg(args, char*); \
            int len = strnlen(val, mul); \
            memcpy(U8(buffer) + cur_size, val, sizeof(char) * len); \
            memset(U8(buffer) + cur_size + len, 0, mul - len); \
            cur_size += sizeof(char) * mul; \
        } break;
        case 'p':{ \
            uint8_t *val = va_arg(args, uint8_t*); \
            memcpy(U8(buffer) + cur_size, val, sizeof(uint8_t) * mul); \
            cur_size += sizeof(uint8_t) * mul; \
        } break;
        default: return -1;
        }
        
    }

    return size;
}

static int unpack(void *buffer, unsigned buffer_size, const char *fmt, va_list args, bool align, bool inverse){
    int size = calcsize_aligned_native(fmt);
    if(size < 0 || (unsigned)size > buffer_size){
        return size;
    }

    int cur_size = 0;
    int repeat = 0;
    int mul = 1;

    for(char ch = *fmt; ch; ch = *++fmt){
        if(isdigit(ch)){
            repeat *= 10;
            repeat += ch - '0';
            continue;
        }

        mul = repeat ? repeat : 1;
        repeat = 0;

        switch (ch){
        #define FMT(CH, _, NATIVE_TYPE, __, VA_GET_LOCATION, ...) \
        case CH:{ \
            for(int i = 0; i < mul; i++){ \
                if(align) cur_size = MC_ALIGN(alignof(NATIVE_TYPE), cur_size); \
                void *location = VA_GET_LOCATION(args); \
                if(location == NULL) continue; \
                memcpy(location, &U8(buffer)[cur_size], sizeof(NATIVE_TYPE)); \
                if(inverse) inverse_endian(location, sizeof(NATIVE_TYPE)); \
                cur_size += sizeof(NATIVE_TYPE); \
            } \
        } break;
        ITER_BASIC_FMT()
        ITER_NATIVE_FMT()
        #undef FMT

        case 's':
        case 'p':{ \
            memcpy(va_arg(args, void*), &U8(buffer)[cur_size], mul); \
            cur_size += mul; \
        } break;
        default: return -1;
        }
    }

    return size;
}

static void inverse_endian(void *data, size_t size){
    for(uint8_t *beg = data, *end = beg + size - 1; beg < end; beg++, end--){
        uint8_t tmp = *beg;
        *beg = *end;
        *end = tmp;
    }
}
