#ifndef MC_DATA_VECTOR_H
#define MC_DATA_VECTOR_H

#include <mc/util/minmax.h>

#include <stddef.h>
#include <malloc.h>
#include <stdint.h>
#include <memory.h>

#define MC_DEFINE_VECTOR(NAME, TYPE) typedef struct NAME { \
    TYPE *end; \
    TYPE *capacity_end; \
    TYPE beg[]; \
} NAME

#define MC_VECTOR_SIZE(VECTOR) ((VECTOR) ? (VECTOR)->end - (VECTOR)->beg : 0)
#define MC_VECTOR_EMPTY(VECTOR) (VECTOR_SIZE(VECTOR) == 0)
#define MC_VECTOR_DATA(VECTOR) ((VECTOR) ? (VECTOR)->beg : NULL)

#define MC_VECTOR_FREE(VECTOR) if((VECTOR) != NULL) free((VECTOR))

#define MC_VECTOR_EACH(VECTOR, ELEMENT) \
    if((VECTOR)) \
    for(ELEMENT = (VECTOR)->beg; ELEMENT != (VECTOR)->end; ELEMENT++)

#define MC_VECTOR_PUSHN(VECTOR, SIZE, ELEMENTS) \
    __vector_push_bytes(VECTOR, SIZE * sizeof(ELEMENTS[0]), ELEMENTS)

#define MC_VECTOR_PUSH_ARRAY(VECTOR, ARRAY) \
    __vector_push_bytes(VECTOR, sizeof(ARRAY), &ARRAY[0])

inline void *__vector_push_bytes(void *vector, size_t size, const void *data){
    struct{
        uint8_t *end;
        uint8_t *capacity_end;
        uint8_t beg[];
    } *v = vector;

    if(v == NULL || v->end + size > v->capacity_end){
        size_t prev_size = v ? v->end - v->beg : 0;
        size_t prev_capacity = v ? v->capacity_end - v->beg : 0;
        size_t new_capacity = MAX(prev_size + size, prev_capacity * 2 + size);

        v = realloc(v, sizeof(*v) + new_capacity);
        if(!v){
            return v;
        }

        v->end = v->beg + prev_size;
        v->capacity_end = v->beg + new_capacity;
    }

    memcpy(v->end, data, size);
    v->end += size;
    return v;
}

#endif // MC_DATA_VECTOR_H