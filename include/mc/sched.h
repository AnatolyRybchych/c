#ifndef MC_SCHED_H
#define MC_SCHED_H

#include <mc/error.h>
#include <mc/time.h>

#include <stdbool.h>

typedef struct MC_Sched MC_Sched;
typedef struct MC_Task MC_Task;

typedef unsigned MC_TaskStatus;
enum MC_TaskStatus{
    MC_TASK_DONE,
    MC_TASK_SUSPEND,
    MC_TASK_CONTINUE,
};

MC_Error mc_sched_new(MC_Sched **sched);
void mc_sched_delete(MC_Sched *sched);
MC_TaskStatus mc_sched_continue(MC_Sched *sched);
void mc_sched_run(MC_Sched *sched, MC_Time suspend);
bool mc_sched_is_terminating(MC_Sched *sched);

MC_Error mc_sched_task(MC_Sched *sched, MC_Task **task, MC_Time timeout, MC_TaskStatus (*do_some)(MC_Task *this), unsigned context_size, const void *context);
MC_Error mc_run_task(MC_Sched *sched, MC_Task **task, MC_TaskStatus (*do_some)(MC_Task *this), unsigned context_size, const void *context);
MC_Error mc_run_task_after(MC_Task *prev, MC_Task **task, MC_TaskStatus (*do_some)(MC_Task *this), unsigned context_size, const void *context);
void *mc_task_data(MC_Task *task, unsigned *size);
MC_Sched *mc_task_sched(MC_Task *task);
MC_Error mc_task_delay(MC_Task *task, MC_Time delay);
void mc_task_release(MC_Task *task);
void mc_task_allow_hard_terminating(MC_Task *task);

#endif // MC_SCHED_H
