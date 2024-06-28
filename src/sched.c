#include <mc/sched.h>
#include <mc/data/pqueue.h>
#include <mc/util/assert.h>

#include <stddef.h>
#include <stdint.h>
#include <malloc.h>
#include <memory.h>
#include <stdarg.h>

#define TASK_NODE(TASK) ((TaskNode*)((uint8_t*)TASK - offsetof(TaskNode, task)))
#define TASK(NODE) ((MC_Task*)(&(NODE)->task))

typedef struct TaskNode TaskNode;
typedef struct TaskHeader TaskHeader;
typedef struct RunAfterAllTaskData RunAfterAllTaskData;

typedef unsigned SchedFlags;
enum SchedFlags{
    SCHED_TERMINATING = 1 << 0,
};

typedef unsigned TaskFlags;
enum TaskFlags{
    TASK_HARD_TERMINATE = 1 << 0,
    TASK_SCHDULED = 1 << 1,
    TASK_DELAYED = 1 << 2,
    TASK_TODELETE = 1 << 3,
    TASK_DONE = 1 << 4
};

enum InternalTaskStatus{
    TASK_INTERNAL_STATUS_DISAPPEAR = __MC_TASK_STATUS_COUNT,
};

struct TaskHeader{
    TaskFlags flags;
    size_t ref_count;
    MC_Sched *scheduler;
    MC_Time scheduled_on;
    MC_TaskStatus (*do_some)(MC_Task *task);
};

struct MC_Task{
    TaskHeader hdr;
    size_t buffer_size;
    uint8_t buffer[];
};

struct TaskNode{
    TaskNode *next;
    TaskNode *pending;
    size_t buffer_capacity;

    // MC_Task
    TaskHeader task;
    size_t buffer_size;
    uint8_t buffer[];
};

struct MC_Sched{
    SchedFlags flags;
    MC_Time suspend;
    MC_Time now;

    TaskNode *task_under_execution;

    MC_PQueue *scheduled_tasks;
    TaskNode *pre_active;
    TaskNode *active;
    TaskNode *finished, *finished_last;
    TaskNode *garbage;
};

struct RunAfterAllTaskData{
    TaskNode *task;
    size_t prev_cnt;
};

static TaskNode *create_task(MC_Sched *sched, MC_TaskStatus (*do_some)(MC_Task *this), unsigned context_size, const void *context);

static void set_pending_all(TaskNode *task, TaskNode *dependent);
static void set_pre_active(MC_Sched *sched, TaskNode *task);
static void set_active(MC_Sched *sched, TaskNode *task);
static void set_active_all(MC_Sched *sched, TaskNode *task);
static void set_finished(MC_Sched *sched, TaskNode *finished);
static void set_garbage(MC_Sched *sched, TaskNode *garbage);

static MC_Error schedule(MC_Sched *sched, TaskNode *task);
static void reschedule_active_tasks(MC_Sched *sched);
static void flush_finished_tasks(MC_Sched *sched);
static bool active_task_ready(TaskNode *task, const MC_Time *now);
static void activate_scheduled_tasks(MC_Sched *sched);
static void activate_pre_active_tasks(MC_Sched *sched);
static MC_TaskStatus terminate_task(MC_Task *task);
static int time_cmp(const void *lhs, const void *rhs);
static MC_TaskStatus do_some(TaskNode *task);
static MC_TaskStatus run_after_all(MC_Task *task);

MC_Error mc_sched_new(MC_Sched **sched){
    MC_Time now;
    MC_Error error = mc_gettime(MC_GETTIME_SINCE_BOOT, &now);
    if(error != MCE_OK){
        return error;
    }

    MC_Sched *new = malloc(sizeof(MC_Sched));
    *sched = new;
    if(new == NULL){
        return MCE_OUT_OF_MEMORY;
    }

    memset(new, 0, sizeof(MC_Sched));
    new->now = now;

    new->scheduled_tasks = mc_pqueue_create(10, time_cmp);
    if(new->scheduled_tasks == NULL){
        return MCE_OUT_OF_MEMORY;
    }

    return MCE_OK;
}

void mc_sched_delete(MC_Sched *sched){
    if(sched == NULL){
        return;
    }

    sched->flags |= SCHED_TERMINATING;
    while(sched->active){
        for(TaskNode *task = sched->active; task; task = task->next){
            if(task->task.flags & TASK_HARD_TERMINATE){
                task->task.do_some = terminate_task;
            }
        }

        mc_sched_continue(sched);
    }

    flush_finished_tasks(sched);

    for(TaskNode *node, *next; sched->garbage;){
        node = sched->garbage;
        next = node->next;
        free(node);
        sched->garbage = next;
    }

    free(sched);
}

MC_TaskStatus mc_sched_continue(MC_Sched *sched){
    MC_TaskStatus res = MC_TASK_SUSPEND;

    flush_finished_tasks(sched);

    mc_gettime(MC_GETTIME_SINCE_BOOT, &sched->now);

    reschedule_active_tasks(sched);
    activate_scheduled_tasks(sched);

    for(TaskNode *task = sched->active, **own = &sched->active; task;){
        if(!active_task_ready(task, &sched->now)){
            own = &task->next;
            task = *own;
            continue;;
        }

        switch (do_some(task)){
        case MC_TASK_DONE:
            task->task.flags |= TASK_DONE;
            *own = task->next ? task->next : task->pending;
            set_finished(sched, task);
            task = *own;
            break;
        case MC_TASK_CONTINUE:
            res = MC_TASK_CONTINUE;
            // FALLTHROUGH
        case MC_TASK_SUSPEND:
            own = &task->next;
            task = *own;
            break;
        case TASK_INTERNAL_STATUS_DISAPPEAR:
            *own = task->next;
            task = *own;
            break;
        }
    }

    activate_pre_active_tasks(sched);

    if(sched->active) return res;
    else if(mc_pqueue_getv(sched->scheduled_tasks)) return MC_TASK_SUSPEND;
    else return MC_TASK_DONE;
}

void mc_sched_run(MC_Sched *sched){
    while (true) switch (mc_sched_continue(sched)){
    case MC_TASK_DONE:
        return;
    case MC_TASK_SUSPEND:
        mc_sleep(&sched->suspend);
        break;
    case MC_TASK_CONTINUE:
        break;
    }
}

bool mc_sched_is_terminating(MC_Sched *sched){
    return sched->flags & SCHED_TERMINATING;
}

MC_Error mc_run_task(MC_Sched *sched, MC_Task **task, MC_TaskStatus (*do_some)(MC_Task *this), unsigned context_size, const void *context){
    if(sched->flags & SCHED_TERMINATING){
        return MCE_BUSY;
    }

    TaskNode *new = create_task(sched, do_some, context_size, context);
    if(new == NULL){
        *task = NULL;
        return MCE_OUT_OF_MEMORY;
    }
    else{
        set_active(sched, new);
        *task = TASK(new);
        return MCE_OK;
    }
}

MC_Error mc_sched_task(MC_Sched *sched, MC_Task **task, MC_Time timeout, MC_TaskStatus (*do_some)(MC_Task *this), unsigned context_size, const void *context){
    if(sched->flags & SCHED_TERMINATING){
        *task = NULL;
        return MCE_BUSY;
    }

    MC_Time scheduled_on;
    MC_Error status = mc_timesum(&sched->now, &timeout, &scheduled_on);
    if(status != MCE_OK){
        *task = NULL;
        return status;
    }

    TaskNode *new = create_task(sched, do_some, context_size, context);
    if(new == NULL){
        *task = NULL;
        return MCE_OUT_OF_MEMORY;
    }

    new->task.scheduled_on = scheduled_on;
    status = schedule(sched, new);
    if(status != MCE_OK){
        *task = NULL;
        free(new);
        return MCE_OUT_OF_MEMORY;
    }

    *task = TASK(new);

    return MCE_OK;
}

MC_Error mc_run_task_after(MC_Task *prev, MC_Task **task, MC_TaskStatus (*do_some)(MC_Task *this), unsigned context_size, const void *context){
    MC_Sched *sched = prev->hdr.scheduler;
    if(sched->flags & SCHED_TERMINATING){
        *task = NULL;
        return MCE_BUSY;
    }

    TaskNode *new = create_task(sched, do_some, context_size, context);
    if(new == NULL){
        *task = NULL;
        return MCE_OUT_OF_MEMORY;
    }
    else{
        *task = TASK(new);
        set_pending_all(TASK_NODE(prev), new);
        return MCE_OK;
    }
}

MC_Error mc_run_task_after_all(MC_Sched *sched, unsigned prev_cnt, MC_Task *prev[prev_cnt], MC_Task **task,
    MC_TaskStatus (*do_some)(MC_Task *this), unsigned context_size, const void *context)
{
    if(sched->flags & SCHED_TERMINATING){
        *task = NULL;
        return MCE_BUSY;
    }

    if(prev_cnt == 0){
        return mc_run_task(sched, task, do_some, context_size, context);
    }

    if(prev_cnt == 1){
        return mc_run_task_after(*prev, task, do_some, context_size, context);
    }

    TaskNode *run_after = create_task(sched, do_some, context_size, context);
    if(run_after == NULL){
        *task = NULL;
        return MCE_OUT_OF_MEMORY;
    }

    TaskNode *waiting_task = create_task(sched, run_after_all, sizeof(RunAfterAllTaskData) + sizeof(MC_Task*[prev_cnt]), NULL);
    if(waiting_task == NULL){
        *task = NULL;
        set_garbage(sched, run_after);
        return MCE_OUT_OF_MEMORY;
    }

    *task = TASK(run_after);
    mc_task_unref(TASK(waiting_task));

    RunAfterAllTaskData *waiting_data = mc_task_data(TASK(waiting_task), NULL);
    waiting_data->task = run_after;
    waiting_data->prev_cnt = prev_cnt;

    for(MC_Task **prev_task = prev; prev_task != &prev[prev_cnt]; prev_task++){
        set_pending_all(TASK_NODE(*prev_task), waiting_task);
    }

    return MCE_OK;
}

void *mc_task_data(MC_Task *task, unsigned *size){
    if(size){
        *size = task->buffer_size;
    }

    return task->buffer;
}

MC_Sched *mc_task_sched(MC_Task *task){
    return task->hdr.scheduler;
}

MC_Error mc_task_delay(MC_Task *task, MC_Time delay){
    if(task->hdr.flags & TASK_SCHDULED){
        return MCE_BUSY;
    }

    MC_Time schedule_on;
    MC_Error error = mc_timesum(&task->hdr.scheduler->now, &delay, &schedule_on);
    if(error != MCE_OK){
        return error;
    }

    task->hdr.scheduled_on = schedule_on;
    task->hdr.flags |= TASK_DELAYED;
    return MCE_OK;
}

MC_Task *mc_task_ref(MC_Task *task){
    task->hdr.ref_count++;
    return task;
}

void mc_task_unref(MC_Task *task){
    MC_ASSERT_FAULT(task->hdr.ref_count > 0);
    task->hdr.ref_count--;

    if(task->hdr.ref_count){
        return;
    }

    if(task->hdr.flags & TASK_TODELETE){
        free(task);
    }
}

MC_Error mc_task_wait_all(const MC_Time *timeout, MC_Task *task, ...){
    MC_Sched *sched = task->hdr.scheduler;

    MC_Time wait_until;
    if(timeout && mc_timesum(&sched->now, timeout, &wait_until) == MCE_OVERFLOW){
        wait_until = (MC_Time){.sec = ~0LLU};
    }

    va_list tasks;
    va_start(tasks, task);

    for(MC_Task *cur = task; task; task = va_arg(tasks, MC_Task*)){
        while (!(cur->hdr.flags & TASK_DONE)){
            switch (mc_sched_continue(sched)){
            case MC_TASK_DONE:
                va_end(tasks);
                return MCE_OK;
            case MC_TASK_SUSPEND:
                mc_sleep(&sched->suspend);
                break;
            case MC_TASK_CONTINUE:
                break;
            }

            if(timeout && mc_timecmp(&wait_until, &sched->now) < 0){
                va_end(tasks);
                return MCE_TIMEOUT;
            }
        }
    }

    va_end(tasks);
    return MCE_OK;
}

void mc_task_allow_hard_terminating(MC_Task *task){
    task->hdr.flags |= TASK_HARD_TERMINATE;
}

static TaskNode *create_task(MC_Sched *sched, MC_TaskStatus (*do_some)(MC_Task *this), unsigned context_size, const void *context){
    TaskNode *new;
    if (sched->garbage){
        new = sched->garbage;
        if(context_size > new->buffer_capacity){
            new = realloc(new, sizeof(TaskNode) + context_size);
            if(new == NULL){
                //TODO: clean up some garbage and try again
                return NULL;
            }

            sched->garbage = sched->garbage->next;
            new->buffer_capacity = context_size;
        }
    }
    else{
        new = malloc(sizeof(TaskNode) + context_size);
        if(new == NULL){
            return NULL;
        }

        *new = (TaskNode){0};
        new->buffer_capacity = context_size;
    }

    new->next = NULL;
    new->pending = NULL;
    new->task.flags = 0;
    new->task.ref_count = 1;
    new->task.scheduler = sched;
    new->task.do_some = do_some;
    new->buffer_size = context_size;

    if(context)
        memcpy(new->buffer, context, context_size);

    return new;
}

static void set_active(MC_Sched *sched, TaskNode *task){
    task->next = sched->active;
    sched->active = task;
}

static void set_active_all(MC_Sched *sched, TaskNode *task){
    for(TaskNode *cur = task, *next = cur ? cur->next : NULL; cur; cur = next, next = next ? next->next : NULL){
        set_active(sched, cur);
    }
}

static void set_finished(MC_Sched *sched, TaskNode *finished){
    finished->next = sched->finished;
    sched->finished = finished;
}

static void set_garbage(MC_Sched *sched, TaskNode *garbage){
    garbage->next = sched->garbage;
    sched->garbage = garbage;
}

static void set_pre_active(MC_Sched *sched, TaskNode *task){
    task->next = sched->pre_active;
    sched->pre_active = task;
}

static MC_Error schedule(MC_Sched *sched, TaskNode *task){
    MC_PQueue *scheduled_tasks = mc_pqueue_enqueue(sched->scheduled_tasks, task);
    task->task.flags &= ~TASK_DELAYED;
    task->task.flags |= TASK_SCHDULED;
    if(!scheduled_tasks){
        return MCE_OUT_OF_MEMORY;
    }
    
    sched->scheduled_tasks = scheduled_tasks;
    return MCE_OK;
}

static bool active_task_ready(TaskNode *task, const MC_Time *now){
    if(task->task.flags & TASK_SCHDULED){
        return mc_timecmp(now, &task->task.scheduled_on) >= 0;
    }

    return true;
}

static void activate_scheduled_tasks(MC_Sched *sched){
    for(TaskNode *scheduled = mc_pqueue_getv(sched->scheduled_tasks);
    scheduled && mc_timecmp(&sched->now, &scheduled->task.scheduled_on) >= 0;
    scheduled = mc_pqueue_getv(sched->scheduled_tasks)){
        TaskNode *active = mc_pqueue_dequeuev(sched->scheduled_tasks);
        active->task.flags &= ~(TASK_SCHDULED | TASK_DELAYED);
        set_active_all(sched, active);
    }
}

static void activate_pre_active_tasks(MC_Sched *sched){
    if(sched->pre_active){
        set_active_all(sched, sched->pre_active);
        sched->pre_active = NULL;
    }
}

static void flush_finished_tasks(MC_Sched *sched){
    for(TaskNode *cur = sched->finished, *next; cur; cur = next){
        next = cur->next;

        if(cur->task.ref_count == 0){
            set_garbage(sched, cur);
        }
        else{
            cur->task.flags |= TASK_TODELETE;
            cur->next = NULL;
        }
    }

    sched->finished = NULL;
}

static void reschedule_active_tasks(MC_Sched *sched){
    TaskNode **own = &sched->active;
    for(TaskNode *active = sched->active;active;){
        if(active->task.flags & (TASK_DELAYED | TASK_SCHDULED)){
            active->task.flags &= ~(TASK_DELAYED | TASK_SCHDULED);

            if(schedule(sched, active) == MCE_OK){
                *own = active->next;
                active->next = NULL;
                active = *own;
                continue;
            }
        }

        own = &active->next;
        active = *own;
    }
}

static MC_TaskStatus terminate_task(MC_Task *task){
    (void)task;
    return MC_TASK_DONE;
}

static int time_cmp(const void *lhs, const void *rhs){
    return mc_timecmp(lhs, rhs);
}

static MC_TaskStatus do_some(TaskNode *task){
    TaskNode *outher = task->task.scheduler->task_under_execution;
    task->task.scheduler->task_under_execution = task;
    MC_TaskStatus result = task->task.do_some(TASK(task));
    task->task.scheduler->task_under_execution = outher;
    return result;
}

static MC_TaskStatus run_after_all(MC_Task *task){
    RunAfterAllTaskData *data = mc_task_data(task, NULL);

    MC_ASSERT_BUG(data->prev_cnt > 0);
    data->prev_cnt--;
    if(data->prev_cnt == 0){
        set_pre_active(mc_task_sched(task), data->task);
        return MC_TASK_DONE;
    }

    return TASK_INTERNAL_STATUS_DISAPPEAR;
}

static void set_pending_all(TaskNode *task, TaskNode *dependent){
    if(task->pending){
        TaskNode *last_dependend;
        for(last_dependend = dependent; last_dependend; last_dependend = last_dependend->next);
        last_dependend->next = task->pending;
        task->pending = last_dependend;
    }
    else{
        task->pending = dependent;
    }
}
