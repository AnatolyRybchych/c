#ifndef MC_DATA_PQUEUE_H
#define MC_DATA_PQUEUE_H

#include <stddef.h>
#include <stdbool.h>

typedef struct MC_PQueue MC_PQueue;

MC_PQueue *mc_pqueue_create(size_t initial_capacity, int (*cmp)(const void *first, const void *second));
MC_PQueue *mc_pqueue_enqueue(MC_PQueue *queue, void *element);
bool mc_pqueue_get(MC_PQueue *queue, void **value);
void *mc_pqueue_getv(MC_PQueue *queue);
bool mc_pqueue_dequeue(MC_PQueue *queue, void **value);
void *mc_pqueue_dequeuev(MC_PQueue *queue);

#endif // MC_DATA_PQUEUE_H
