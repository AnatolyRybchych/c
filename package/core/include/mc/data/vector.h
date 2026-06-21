#ifndef MC_DATA_VECTOR_H
#define MC_DATA_VECTOR_H

#include <mc/util/minmax.h>
#include <mc/util/error.h>
#include <mc/data/alloc.h>

#include <stddef.h>
#include <stdint.h>
#include <memory.h>
#include <string.h>

#define MC_DEFINE_VECTOR(NAME, TYPE) typedef struct NAME { \
    TYPE *end; \
    TYPE *capacity_end; \
    TYPE beg[]; \
} NAME

#define MC_VECTOR_SIZE(VECTOR) ((VECTOR) ? (VECTOR)->end - (VECTOR)->beg : 0)
#define MC_VECTOR_EMPTY(VECTOR) ((VECTOR) ? MC_VECTOR_SIZE(VECTOR) == 0 : false)
#define MC_VECTOR_DATA(VECTOR) ((VECTOR) ? (VECTOR)->beg : NULL)

#define MC_VECTOR_FREE(ALLOC, VECTOR) if((VECTOR) != NULL) mc_free((ALLOC), (VECTOR))

#define MC_VECTOR_EACH(VECTOR, ELEMENT) \
    if((VECTOR)) \
    for(ELEMENT = (VECTOR)->beg; ELEMENT != (VECTOR)->end; ELEMENT++)

#define MC_VECTOR_PUSHN(ALLOC, VECTOR, COUNT, ELEMENTS) \
    __vector_push_bytes((ALLOC), (void**)&(VECTOR), (COUNT) * sizeof((ELEMENTS)[0]), (ELEMENTS))

#define MC_VECTOR_PUSH_ARRAY(ALLOC, VECTOR, ARRAY) \
    __vector_push_bytes((ALLOC), (void**)&(VECTOR), sizeof(ARRAY), &(ARRAY)[0])

#define MC_VECTOR_RESERVE(ALLOC, VECTOR, COUNT) \
    __vector_reserve_bytes((ALLOC), (void**)&(VECTOR), sizeof((VECTOR)->beg[0]) * (COUNT))

#define MC_VECTOR_ERASE(VECTOR, IDX, COUNT) ((VECTOR) ? __vector_erase_bytes((VECTOR), (IDX) * sizeof *(VECTOR)->beg, (COUNT) * sizeof *(VECTOR)->beg) : (void)0)

inline void __vector_erase_bytes(void *vector, size_t idx, size_t size){
    struct{
        uint8_t *end;
        uint8_t *capacity_end;
        uint8_t beg[];
    } *v = vector;

    if(size > (size_t)(v->end - v->beg)){
        size = v->end - v->beg;
    }

    if(size != 0){
        memmove(v->beg, v->beg + idx, size);
        v->end -= size;
    }
}

inline MC_Error __vector_reserve_bytes(MC_Alloc *alloc, void **vector, size_t size){
    struct VectorHeader{
        uint8_t *end;
        uint8_t *capacity_end;
        uint8_t beg[];
    } *v = *vector;

    if(v != NULL && v->end + size <= v->capacity_end){
        return MCE_OK;
    }

    size_t prev_size = v ? (size_t)(v->end - v->beg) : 0;
    size_t prev_capacity = v ? (size_t)(v->capacity_end - v->beg) : 0;
    size_t new_capacity = MAX(prev_size + size, prev_capacity * 2 + size);

    void *resized = v;
    MC_Error status = mc_realloc(alloc, &resized, sizeof(*v) + new_capacity);
    if(status == MCE_NOT_SUPPORTED){
        resized = NULL;
        MC_RETURN_ERROR(mc_alloc(alloc, sizeof(*v) + new_capacity, &resized));

        if(v != NULL){
            memcpy(resized, v, sizeof(*v) + prev_size);
            mc_free(alloc, v);
        }
    }
    else{
        MC_RETURN_ERROR(status);
    }

    v = resized;
    v->end = v->beg + prev_size;
    v->capacity_end = v->beg + new_capacity;

    *vector = v;
    return MCE_OK;
}

inline MC_Error __vector_push_bytes(MC_Alloc *alloc, void **vector, size_t size, const void *data){
    MC_RETURN_ERROR(__vector_reserve_bytes(alloc, vector, size));

    struct{
        uint8_t *end;
        uint8_t *capacity_end;
        uint8_t beg[];
    } *v = *vector;

    memcpy(v->end, data, size);
    v->end += size;

    return MCE_OK;
}

#endif // MC_DATA_VECTOR_H
