#ifndef MC_SCHED_H
#define MC_SCHED_H

#include <mc/error.h>
#include <mc/time.h>
#include <mc/util/enum.h>

#include <stdbool.h>
#include <stddef.h>

#define MC_ITER_TASK_STATUS(CB, ...) \
    CB(DONE, ##__VA_ARGS__) \
    CB(SUSPEND, ##__VA_ARGS__) \
    CB(CONTINUE, ##__VA_ARGS__) \

typedef struct MC_Sched MC_Sched;
typedef struct MC_Task MC_Task;

typedef unsigned MC_TaskStatus;
enum MC_TaskStatus{
    MC_ITER_TASK_STATUS(MC_DEFINE_ENUM_ITEM, MC_TASK_)
    __MC_TASK_STATUS_COUNT
};

MC_Error mc_sched_new(MC_Sched **sched);
void mc_sched_delete(MC_Sched *sched);

MC_TaskStatus mc_sched_continue(MC_Sched *sched);
void mc_sched_run(MC_Sched *sched);

bool mc_sched_is_terminating(MC_Sched *sched);
void mc_sched_set_suspend_interval(MC_Sched *sched, MC_Time suspend);

MC_Error mc_sched_task(MC_Sched *sched, MC_Task **task, MC_Time timeout, MC_TaskStatus (*do_some)(MC_Task *this), unsigned context_size, const void *context);
MC_Error mc_run_task(MC_Sched *sched, MC_Task **task, MC_TaskStatus (*do_some)(MC_Task *this), unsigned context_size, const void *context);

MC_Error mc_task_delay(MC_Task *task, MC_Time delay);

void *mc_task_data(MC_Task *task, unsigned *size);
MC_Sched *mc_task_sched(MC_Task *task);

MC_Task *mc_task_ref(MC_Task *task);
void mc_task_unref(MC_Task *task);

void mc_task_set_handler(MC_Task *task, MC_TaskStatus (*do_some)(MC_Task *this));

/// @brief blocks until all [task...] and all mc_task_wait called during this call done
/// @param task... list of tasks to wait, last SHOULD be NULL
/// @param timeout if NULL, then timeout is infinite
/// @return MCE_TIMEOUT if exited due to timeout
MC_Error mc_task_wait(const MC_Time *timeout, MC_Task *task, ...);

/// @param task task that can be silently terminated
void mc_task_allow_hard_terminating(MC_Task *task);

const char *mc_task_status_str(MC_TaskStatus status);

#endif // MC_SCHED_H
