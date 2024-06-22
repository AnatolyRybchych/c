#include <stdio.h>

#include <mc.h>
#include <mc/dlib.h>
#include <mc/sched.h>

#include <stdbool.h>

static MC_TaskStatus count(MC_Task *task){
    int *value = mc_task_data(task, NULL);
    printf("count: %i\n", *value);

    if(*value == 0){
        return MC_TASK_DONE;
    }

    *value -= 1;

    return MC_TASK_CONTINUE;
}

int main(){
    MC_Sched *sched;
    mc_sched_new(&sched);

    int initial_count = 10;
    MC_Task *counter1, *counter2, *counter3;
    mc_run_task(sched, &counter1, count, sizeof(int), &initial_count);
    mc_run_task(sched, &counter2, count, sizeof(int), &initial_count);
    mc_run_task_after(counter2, &counter3, count, sizeof(int), &initial_count);

    mc_sched_run(sched, 1);
    mc_sched_delete(sched);
}

