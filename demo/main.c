#include <stdio.h>

#include <mc/dlib.h>
#include <mc/sched.h>
#include <mc/time.h>
#include <mc/data/pqueue.h>

#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

MC_TaskStatus greet(MC_Task *task){
    const char **name = mc_task_data(task, NULL);
    printf("%s\n", *name);
    mc_task_delay(task, (MC_Time){.sec = 1});
    return MC_TASK_SUSPEND;
}

int main(){
    MC_Sched *sched;
    mc_sched_new(&sched);

    MC_Task *task;
    char *test = "qwerty";
    mc_sched_task(sched, &task, (MC_Time){.sec = 1}, greet, sizeof(char*), &test);

    mc_sched_run(sched, (MC_Time){
        .sec = 0,
        .nsec = 10000
    });
}

