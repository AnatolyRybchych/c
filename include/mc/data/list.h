#ifndef MC_DATA_LIST_H
#define MC_DATA_LIST_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define MC_LIST_FOR(TYPE, LIST, ELEMENT) \
    for(TYPE *ELEMENT = (LIST); ELEMENT; ELEMENT = (TYPE*)ELEMENT->next)

#define MC_LIST_FORS(TYPE, LIST, ELEMENT) \
    if((LIST)) for(TYPE *ELEMENT##_prev = NULL, **ELEMENT##_location = &(LIST), *ELEMENT = *ELEMENT##_location, *__##ELEMENT##_tmp; \
    *ELEMENT##_location; __##ELEMENT##_tmp = ELEMENT##_prev, ELEMENT##_prev = *ELEMENT##_location, \
    ELEMENT##_location = __##ELEMENT##_tmp ? (TYPE**)&__##ELEMENT##_tmp->next->next : &__##ELEMENT##_tmp, ELEMENT = *ELEMENT##_location)

#define MC_LIST_FOR_LOCATIONS(TYPE, LIST, ELEMENT) \
    if((LIST)) for(TYPE *ELEMENT##_prev = NULL, **ELEMENT##_location = &(LIST), *__##ELEMENT##_tmp; \
    *ELEMENT##_location; __##ELEMENT##_tmp = ELEMENT##_prev, ELEMENT##_prev = *ELEMENT##_location, \
    ELEMENT##_location = __##ELEMENT##_tmp ? (TYPE**)&__##ELEMENT##_tmp->next->next : &__##ELEMENT##_tmp)

typedef struct MC_List MC_List;

struct MC_List{
    MC_List *next;
};

inline bool mc_list_empty(const void *list){
    return list == NULL;
}

inline void mc_list_add(void *own_location, void *node){
    if(node == NULL){
        return;
    }

    MC_List *node_last = node;
    while(node_last->next){
        node_last = node_last->next;
    }

    node_last->next = *(MC_List**)own_location;
    *(MC_List**)own_location = node;
}

inline void *mc_list_add_after(void *prev, void *node){
    if(prev == NULL){
        return node;
    }

    if(node == NULL){
        return prev;
    }

    MC_List *node_last = node;
    while(node_last->next){
        node_last->next = node_last->next->next;
    }

    MC_List *p = (MC_List*)prev;
    node_last->next = p->next;
    p->next = node;
    return node;
}

inline void *mc_list_remove(void *node_in_owner){
    MC_List *to_remove = *(MC_List**)node_in_owner;
    if(to_remove){
        *(MC_List**)node_in_owner = to_remove->next;
        to_remove->next = NULL;
        return to_remove;
    }

    return NULL;
}

#endif // MC_DATA_LIST_H
