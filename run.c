#include <stdio.h>

#include <mc.h>
#include <mc/dlib.h>
#include <mc/sched.h>
#include <mc/time.h>

#include <stdbool.h>

static MC_TaskStatus count(MC_Task *task){
    MC_Time time;
    if(mc_gettime(MC_GETTIME_SINCE_BOOT, &time)){
        return MC_TASK_DONE;
    }

    MC_Time *prev_time = mc_task_data(task, NULL);
    MC_Time diff;
    mc_timediff(prev_time, &time, &diff);
    for(uint64_t sec = 0; sec < diff.sec; sec++){
        printf("count(%zu)\n", prev_time->sec + sec);
    }

    prev_time->sec += diff.sec;

    return MC_TASK_SUSPEND;
}

int main(){
    MC_Sched *sched;
    mc_sched_new(&sched);

    MC_Time time;
    mc_gettime(MC_GETTIME_SINCE_PROCCESS_START, &time);
    MC_Task *counter;
    mc_run_task(sched, &counter, count, sizeof(time), &time);

    mc_sched_run(sched, 1);
    mc_sched_delete(sched);
}

