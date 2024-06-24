#include <stdio.h>

#include <mc.h>
#include <mc/dlib.h>
#include <mc/sched.h>
#include <mc/time.h>
#include <mc/data/pqueue.h>

#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

int cmp(const void *first, const void *second){
    return strcmp(first, second);
}

int main(){
    MC_PQueue *queue = mc_pqueue_create(0, cmp);
    queue = mc_pqueue_enqueue(queue, "a");
    queue = mc_pqueue_enqueue(queue, "b");
    queue = mc_pqueue_enqueue(queue, "c");
    queue = mc_pqueue_enqueue(queue, "d");
    queue = mc_pqueue_enqueue(queue, "e");
    queue = mc_pqueue_enqueue(queue, "f");
    queue = mc_pqueue_enqueue(queue, "g");


    for(char *s = mc_pqueue_dequeuev(queue); s; s = mc_pqueue_dequeuev(queue)){
        printf("%s\n", s);
    }
}

