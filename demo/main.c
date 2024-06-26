#include <stdio.h>

#include <mc/dlib.h>
#include <mc/sched.h>
#include <mc/time.h>
#include <mc/data/pqueue.h>

#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

typedef struct Greet Greet;

struct Greet{
    char *name;
    uint32_t cnt;
};

MC_TaskStatus greet(MC_Task *task){
    Greet *data = mc_task_data(task, NULL);
    printf("%s\n", data->name);
    // mc_task_delay(task, (MC_Time){.sec = 1});
    return MC_TASK_DONE;
}

int main(){
    MC_Sched *sched;
    mc_sched_new(&sched);

    MC_Task *task;
    Greet greet_data = {.name = "QWERTY", .cnt = 3};
    mc_sched_task(sched, &task, (MC_Time){.sec = 1}, greet, sizeof(greet_data), &greet_data);
    mc_task_release(task);

    mc_sched_run(sched, (MC_Time){
        .sec = 0,
        .nsec = 10000
    });

    mc_sched_delete(sched);

}

