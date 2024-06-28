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
    mc_task_delay(task, (MC_Time){.sec = 1});
    return --data->cnt ? MC_TASK_CONTINUE :MC_TASK_DONE;
}

int main(){
    MC_Sched *sched;
    mc_sched_new(&sched);

    Greet greet1 = {.name = "GREET 1", .cnt = 3};
    Greet greet2 = {.name = "GREET 2", .cnt = 5};
    Greet greet3 = {.name = "GREET 3", .cnt = 3};

    MC_Task *task1;
    MC_Task *task2;
    MC_Task *task3;
    mc_sched_task(sched, &task1, (MC_Time){.sec = 1}, greet, sizeof(greet1), &greet1);
    mc_sched_task(sched, &task2, (MC_Time){.sec = 1}, greet, sizeof(greet2), &greet2);
    mc_run_task_after_all(sched, 2, (MC_Task*[2]){task1, task2}, &task3, greet, sizeof(greet3), &greet3);

    mc_task_unref(task1);
    mc_task_unref(task2);
    mc_task_unref(task3);

    mc_sched_run(sched);

    mc_sched_delete(sched);

}

