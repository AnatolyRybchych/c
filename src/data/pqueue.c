#include <mc/data/pqueue.h>

#include <stdint.h>
#include <malloc.h>

#define ROOT (0)
#define AT(IDX) ((queue)->beg[IDX])
#define LEFT(IDX) ((IDX) * 2 + 1)
#define RIGHT(IDX) ((IDX) * 2 + 2)
#define PARENT(IDX) (((IDX) - 1) / 2)

#define SWAP(V1, V2) do { \
    (tmp) = (V1); \
    (V1) = (V2); \
    (V2) = (tmp); \
} while(0)

struct MC_PQueue{
    int (*cmp)(const void *first, const void *second);
    size_t size;
    size_t capacity;
    void *beg[];
};

MC_PQueue *mc_pqueue_create(size_t initial_capacity, int (*cmp)(const void *first, const void *second)){
    MC_PQueue *queue = malloc(sizeof(MC_PQueue) + sizeof(void*[initial_capacity]));
    if(queue == NULL){
        return NULL;
    }

    queue->cmp = cmp;
    queue->size = 0;
    queue->capacity = initial_capacity;

    return queue;
}

MC_PQueue *mc_pqueue_enqueue(MC_PQueue *queue, void *element){
    if(queue->size >= queue->capacity){
        size_t capacity = queue->capacity * 2 + 1;
        queue = realloc(queue, sizeof(MC_PQueue) + sizeof(void*[capacity]));
        if(queue == NULL){
            return NULL;
        }

        queue->capacity = capacity;
    }

    void *tmp;
    AT(queue->size) = element;
    for(size_t cur = queue->size++; cur != ROOT && queue->cmp(AT(PARENT(cur)), AT(cur)) > 0; cur = PARENT(cur)){
        SWAP(AT(PARENT(cur)), AT(cur));
    }

    return queue;
}

bool mc_pqueue_get(MC_PQueue *queue, void **value){
    if(queue->size){
        *value = *queue->beg;
    }

    return queue->size;
}

void *mc_pqueue_getv(MC_PQueue *queue){
    return queue->size ? *queue->beg : NULL;
}

bool mc_pqueue_dequeue(MC_PQueue *queue, void **value){
    if(queue->size == 0){
        return false;
    }

    *value = AT(ROOT);
    AT(ROOT) = AT(--queue->size);

    void *tmp;
    for(size_t cur = ROOT; LEFT(cur) < queue->size;){
        size_t min;

        if(RIGHT(cur) >= queue->size){
            min = LEFT(cur);
        }
        else{
            min = queue->cmp(AT(RIGHT(cur)), AT(LEFT(cur))) <= 0
            ? RIGHT(cur)
            : LEFT(cur);
        }

        if(queue->cmp(AT(cur), AT(min)) <= 0){
            break;
        }

        SWAP(AT(cur), AT(min));
        cur = min;
    }

    return true;
}

void *mc_pqueue_dequeuev(MC_PQueue *queue){
    void *result;
    return mc_pqueue_dequeue(queue, &result) ? result : NULL;
}
