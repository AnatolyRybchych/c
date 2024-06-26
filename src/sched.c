#include <mc/sched.h>
#include <mc/data/pqueue.h>
#include <mc/util/assert.h>

#include <stddef.h>
#include <stdint.h>
#include <malloc.h>
#include <memory.h>
#include <stdarg.h>

#define TASK_NODE(TASK) ((TaskNode*)((uint8_t*)TASK - offsetof(TaskNode, task)))

typedef struct TaskNode TaskNode;
typedef struct TaskHeader TaskHeader;

typedef unsigned SchedFlags;
enum SchedFlags{
    SCHED_TERMINATING = 1 << 0,
};

typedef unsigned TaskFlags;
enum TaskFlags{
    TASK_HARD_TERMINATE = 1 << 0,
    TASK_SCHDULED = 1 << 1,
    TASK_DELAYED = 1 << 2,
    TASK_DETACHED = 1 << 3,
    TASK_TODELETE = 1 << 4,
    TASK_DONE = 1 << 5,
    TASK_IGNORE = 1 << 6,
};

struct TaskHeader{
    TaskFlags flags;
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
    TaskNode *active_tasks;
    TaskNode *active_last;
    TaskNode *finished;
    TaskNode *garbage;
};

static TaskNode *create_task(MC_Sched *sched, MC_TaskStatus (*do_some)(MC_Task *this), unsigned context_size, const void *context);
static void set_active(MC_Sched *sched, TaskNode *task);
static MC_Error schedule(MC_Sched *sched, TaskNode *task);
static void reschedule_active_tasks(MC_Sched *sched);
static void flush_finished_tasks(MC_Sched *sched);
static bool active_task_ready(TaskNode *task, const MC_Time *now);
static void activate_scheduled_tasks(MC_Sched *sched);
static MC_TaskStatus terminate_task(MC_Task *task);
static int time_cmp(const void *lhs, const void *rhs);
static MC_TaskStatus do_some(TaskNode *task);

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
    while(sched->active_tasks){
        for(TaskNode *task = sched->active_tasks; task; task = task->next){
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

    for(TaskNode *task = sched->active_tasks, **own = &sched->active_tasks; task;){
        if(!active_task_ready(task, &sched->now)){
            own = &task->next;
            task = *own;
            continue;;
        }

        switch (do_some(task)){
        case MC_TASK_DONE:
            task->task.flags |= TASK_DONE;
            *own = task->pending ? task->pending : task->next;
            task->next = sched->finished;
            sched->finished = task;
            task = *own;
            break;
        case MC_TASK_CONTINUE:
            res = MC_TASK_CONTINUE;
            // FALLTHROUGH
        case MC_TASK_SUSPEND:
            own = &task->next;
            task = *own;
            break;
        }
    }

    if(sched->active_tasks) return res;
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

void mc_sched_sleep(MC_Sched *sched, MC_Time delay){
    TaskNode *task_under_execution = sched->task_under_execution;
    if(task_under_execution){
        task_under_execution->task.flags |= TASK_IGNORE;
    }

    MC_Time sleep_until;
    if(mc_timesum(&sched->now, &delay, &sleep_until) == MCE_OVERFLOW){
        while (true){
            mc_sleep(&(MC_Time){.sec = 600});
        }
    }

    while (mc_timecmp(&sched->now, &sleep_until) < 0) switch (mc_sched_continue(sched)){
    case MC_TASK_DONE:
    case MC_TASK_SUSPEND:
        mc_sleep(&sched->suspend);
        break;
    case MC_TASK_CONTINUE:
        break;
    }

    if(task_under_execution){
        task_under_execution->task.flags &= ~TASK_IGNORE;
    }
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
        *task = (MC_Task*)&new->task;
        return MCE_OK;
    }
}

MC_Error mc_sched_task(MC_Sched *sched, MC_Task **task, MC_Time timeout, MC_TaskStatus (*do_some)(MC_Task *this), unsigned context_size, const void *context){
    if(sched->flags & SCHED_TERMINATING){
        *task = NULL;
        return MCE_BUSY;
    }

    MC_Time uptime;
    MC_Error status = mc_gettime(MC_GETTIME_SINCE_BOOT, &uptime);
    if(status != MCE_OK){
        *task = NULL;
        return status;
    }

    MC_Time scheduled_on;
    status = mc_timesum(&uptime, &timeout, &scheduled_on);
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

    *task = (MC_Task*)&new->task;

    return MCE_OK;
}

MC_Error mc_run_task_after(MC_Task *prev, MC_Task **task, MC_TaskStatus (*do_some)(MC_Task *this), unsigned context_size, const void *context){
    MC_Sched *sched = prev->hdr.scheduler;
    TaskNode *prev_node = TASK_NODE(prev);
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
        *task = (MC_Task*)&new->task;
        if(prev_node->pending){
            new->next = prev_node->pending;
            prev_node->pending = new;
        }
        else{
            prev_node->pending = new;
        }
        return MCE_OK;
    }
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

    MC_Time now;
    MC_Error error = mc_gettime(MC_GETTIME_SINCE_BOOT, &now);
    if(error != MCE_OK){
        return error;
    }

    if(mc_timecmp(&now, &task->hdr.scheduled_on) < 0){
        task->hdr.scheduled_on = now;
    }

    MC_Time schedule_on;
    error = mc_timesum(&task->hdr.scheduled_on, &delay, &schedule_on);
    if(error != MCE_OK){
        return error;
    }

    task->hdr.scheduled_on = schedule_on;
    task->hdr.flags |= TASK_DELAYED;
    return MCE_OK;
}

void mc_task_release(MC_Task *task){
    if(task->hdr.flags & TASK_TODELETE){
        free(task);
    }
    else{
        task->hdr.flags |= TASK_DETACHED;
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
    new->task.scheduler = sched;
    new->task.do_some = do_some;
    new->buffer_size = context_size;
    memcpy(new->buffer, context, context_size);

    return new;
}

static void set_active(MC_Sched *sched, TaskNode *task){
    if(sched->active_tasks){
        sched->active_last->next = task;
        sched->active_last = task;
    }
    else{
        sched->active_tasks = task;
        sched->active_last = task;
    }
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
    else{
        return !(task->task.flags & TASK_IGNORE);
    }
}

static void activate_scheduled_tasks(MC_Sched *sched){
    for(TaskNode *scheduled = mc_pqueue_getv(sched->scheduled_tasks);
    scheduled && mc_timecmp(&sched->now, &scheduled->task.scheduled_on) >= 0;
    scheduled = mc_pqueue_getv(sched->scheduled_tasks)){
        TaskNode *active = mc_pqueue_dequeuev(sched->scheduled_tasks);
        active->task.flags &= ~(TASK_SCHDULED | TASK_DELAYED);
        set_active(sched, active);
    }
}

static void flush_finished_tasks(MC_Sched *sched){
    for(TaskNode *task = sched->finished, **own = &sched->finished; task;){
        *own = task->next;

        if(task->task.flags & TASK_DETACHED){
            task->next = sched->garbage;
            sched->garbage = task;
        }
        else{
            task->task.flags |= TASK_TODELETE;
        }

        task = *own;
    }
}

static void reschedule_active_tasks(MC_Sched *sched){
    TaskNode **own = &sched->active_tasks;
    for(TaskNode *active = sched->active_tasks;active;){
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
    MC_TaskStatus result = task->task.do_some((MC_Task*)&task->task);
    task->task.scheduler->task_under_execution = outher;
    return result;
}
