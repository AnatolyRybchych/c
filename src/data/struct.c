#include <mc/data/struct.h>
#include <mc/util/assert.h>
#include <mc/util/memory.h>

#include <stdint.h>
#include <stdalign.h>
#include <stdbool.h>
#include <float.h>
#include <ctype.h>
#include <memory.h>

#define U8(PTR) ((uint8_t*)(PTR))



#define ITER_BASIC_FMT() \
    FMT('x', uint8_t,   uint8_t,                int) \
    FMT('c', uint8_t,   char,                   int) \
    FMT('b', uint8_t,   signed char,            int) \
    FMT('B', int8_t,    unsigned char,          int) \
    FMT('?', uint8_t,   bool,                   int) \
    FMT('h', int16_t,   short,                  int) \
    FMT('H', uint16_t,  unsigned short,         int) \
    FMT('e', uint16_t,  uint16_t,               double) \
    FMT('i', int32_t,   int,                    int) \
    FMT('I', uint32_t,  unsigned int,           unsigned int) \
    FMT('l', int32_t,   long int,               long) \
    FMT('L', uint32_t,  unsigned long int,      long) \
    FMT('f', float,     float,                  double) \
    FMT('q', int64_t,   long long int,          long long) \
    FMT('Q', uint64_t,  unsigned long long int, unsigned long long) \
    FMT('d', double,    double,                 double) \

#define ITER_ARR_FMT() \
    FMT('s', uint8_t,   char,   int) \
    FMT('p', uint8_t,   char,   int) 

#define ITER_NATIVE_FMT() \
    FMT('n',,           size_t, size_t) \
    FMT('P',,           void*,  void*) \

#define ITER_FMT() \
    ITER_BASIC_FMT() \
    ITER_ARR_FMT() \
    ITER_NATIVE_FMT()

static int calcsize_aligned_native(const char *fmt);
static int calcsize_unaligned(const char *fmt);
static int calcsize_unaligned_native(const char *fmt);

static int pack_aligned_native(void *buffer, unsigned buffer_size, const char *fmt, va_list args);
static int pack_unaligned_native(void *buffer, unsigned buffer_size, const char *fmt, va_list args);
static int pack_unaligned_native_inverse_endian(void *buffer, unsigned buffer_size, const char *fmt, va_list args);
static int pack_lsb(void *buffer, unsigned buffer_size, const char *fmt, va_list args);
static int pack_msb(void *buffer, unsigned buffer_size, const char *fmt, va_list args);

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
    case '@': return pack_aligned_native(buffer, buffer_size, fmt + 1, args);
    case '=': return pack_unaligned_native(buffer, buffer_size, fmt + 1, args);
    case '<': return pack_lsb(buffer, buffer_size, fmt + 1, args);
    case '>': return pack_msb(buffer, buffer_size, fmt + 1, args);
    case '!': return pack_msb(buffer, buffer_size, fmt + 1, args);
    default : return pack_aligned_native(buffer, buffer_size, fmt, args);
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
            size = ALIGN(alignof(NATIVE_TYPE), size) + alignof(NATIVE_TYPE) * (mul -1) + sizeof(NATIVE_TYPE); \
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

static int pack_aligned_native(void *buffer, unsigned buffer_size, const char *fmt, va_list args){
    int size = calcsize_aligned_native(fmt);
    if(size < 0 || (unsigned)size > buffer_size){
        return size;
    }

    int cur_size;
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
        #define FMT(CH, _, NATIVE_TYPE, VA_TYPE, ...) \
        case CH:{ \
            for(int i = 0; i < mul; i++){ \
                NATIVE_TYPE val = va_arg(args, VA_TYPE); \
                cur_size = ALIGN(alignof(NATIVE_TYPE), cur_size); \
                memcpy(&U8(buffer)[cur_size], &val, sizeof(NATIVE_TYPE)); \
                cur_size += sizeof(NATIVE_TYPE); \
            } \
        } break;
        ITER_BASIC_FMT()
        ITER_NATIVE_FMT()
        #undef FMT

        #define FMT(CH, _, NATIVE_TYPE, ...) \
        case CH:{ \
            NATIVE_TYPE *val = va_arg(args, NATIVE_TYPE*); \
            cur_size = ALIGN(alignof(NATIVE_TYPE), cur_size); \
            memcpy(&U8(buffer)[cur_size], val, alignof(NATIVE_TYPE) * (mul - 1) + sizeof(NATIVE_TYPE)); \
            cur_size += alignof(NATIVE_TYPE) * (mul - 1) + sizeof(NATIVE_TYPE); \
        } break;
        ITER_ARR_FMT()
        #undef FMT
        default: return -1;
        }
        
    }

    return size;
}

static int pack_unaligned_native(void *buffer, unsigned buffer_size, const char *fmt, va_list args){
    int size = calcsize_unaligned_native(fmt);
    if(size < 0 || (unsigned)size > buffer_size){
        return size;
    }

    uint8_t *buf = buffer;
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
        #define FMT(CH, _, NATIVE_TYPE, VA_TYPE, ...) \
        case CH:{ \
            for(int i = 0; i < mul; i++){ \
                NATIVE_TYPE val = va_arg(args, VA_TYPE); \
                memcpy(buf, &val, sizeof(NATIVE_TYPE)); \
                buf += sizeof(NATIVE_TYPE);\
            } \
        } break;
        ITER_BASIC_FMT()
        ITER_NATIVE_FMT()
        #undef FMT

        #define FMT(CH, _, NATIVE_TYPE, ...) \
        case CH:{ \
            NATIVE_TYPE *val = va_arg(args, NATIVE_TYPE*); \
            memcpy(buf, val, sizeof(NATIVE_TYPE) * repeat); \
            buf += sizeof(NATIVE_TYPE) * repeat; \
        } break;
        ITER_ARR_FMT()
        #undef FMT
        default: return -1;
        }
    }

    return size;
}

static int pack_unaligned_native_inverse_endian(void *buffer, unsigned buffer_size, const char *fmt, va_list args){
    int size = calcsize_unaligned_native(fmt);
    if(size < 0 || (unsigned)size > buffer_size){
        return size;
    }

    uint8_t *buf = buffer;
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
        #define FMT(CH, _, NATIVE_TYPE, VA_TYPE, ...) \
        case CH:{ \
            for(int i = 0; i < mul; i++){ \
                NATIVE_TYPE val = va_arg(args, VA_TYPE); \
                memcpy(buf, &val, sizeof(NATIVE_TYPE)); \
                inverse_endian(buf, sizeof(NATIVE_TYPE)); \
                buf += sizeof(NATIVE_TYPE); \
            } \
        } break;
        ITER_BASIC_FMT()
        ITER_NATIVE_FMT()
        #undef FMT

        #define FMT(CH, _, NATIVE_TYPE, ...) \
        case CH:{ \
            NATIVE_TYPE *val = va_arg(args, NATIVE_TYPE*); \
            memcpy(buf, val, sizeof(NATIVE_TYPE) * repeat); \
            buf += sizeof(NATIVE_TYPE) * repeat; \
        } break;
        ITER_ARR_FMT()
        #undef FMT
        default: return -1;
        }
    }

    return size;
}

static int pack_lsb(void *buffer, unsigned buffer_size, const char *fmt, va_list args){
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return pack_unaligned_native(buffer, buffer_size, fmt, args);
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return pack_unaligned_native_inverse_endian(buffer, buffer_size, fmt, args);
#else
    static_assert(false, "pack is not implemented for this byte order");
#endif
}

static int pack_msb(void *buffer, unsigned buffer_size, const char *fmt, va_list args){
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return pack_unaligned_native_inverse_endian(buffer, buffer_size, fmt, args);
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return pack_unaligned_native(buffer, buffer_size, fmt, args);
#else
    static_assert(false, "pack is not implemented for this byte order");
#endif
}

static void inverse_endian(void *data, size_t size){
    for(uint8_t *beg = data, *end = beg + size - 1; beg < end; beg++, end--){
        uint8_t tmp = *beg;
        *beg = *end;
        *end = tmp;
    }
}
