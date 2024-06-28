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

    __MC_TASK_STATUS_COUNT
};

MC_Error mc_sched_new(MC_Sched **sched);
void mc_sched_delete(MC_Sched *sched);
MC_TaskStatus mc_sched_continue(MC_Sched *sched);
void mc_sched_run(MC_Sched *sched);
bool mc_sched_is_terminating(MC_Sched *sched);

MC_Error mc_sched_task(MC_Sched *sched, MC_Task **task, MC_Time timeout, MC_TaskStatus (*do_some)(MC_Task *this), unsigned context_size, const void *context);
MC_Error mc_run_task(MC_Sched *sched, MC_Task **task, MC_TaskStatus (*do_some)(MC_Task *this), unsigned context_size, const void *context);
MC_Error mc_run_task_after(MC_Task *prev, MC_Task **task, MC_TaskStatus (*do_some)(MC_Task *this), unsigned context_size, const void *context);
MC_Error mc_run_task_after_all(MC_Sched *sched, unsigned prev_cnt, MC_Task *prev[prev_cnt], MC_Task **task,
    MC_TaskStatus (*do_some)(MC_Task *this), unsigned context_size, const void *context);
void *mc_task_data(MC_Task *task, unsigned *size);
MC_Sched *mc_task_sched(MC_Task *task);
MC_Error mc_task_delay(MC_Task *task, MC_Time delay);

MC_Task *mc_task_ref(MC_Task *task);
void mc_task_unref(MC_Task *task);

/// @param task... list of tasks to wait, last SHOULD be NULL
/// @param timeout if NULL, then timeout is infinite
/// @return MCE_TIMEOUT if exited due to timeout
MC_Error mc_task_wait_all(const MC_Time *timeout, MC_Task *task, ...);
void mc_task_allow_hard_terminating(MC_Task *task);

#endif // MC_SCHED_H
