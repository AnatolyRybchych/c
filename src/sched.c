#include <mc/sched.h>
#include <mc/data/vector.h>

#include <stddef.h>

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
};

struct TaskHeader{
    TaskFlags flags;
    MC_Sched *scheduler;
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

    TaskNode *active_tasks;
    TaskNode *active_last;
    TaskNode *finished;
    TaskNode *garbage;
};

static TaskNode *create_task(MC_Sched *sched, MC_TaskStatus (*do_some)(MC_Task *this), unsigned context_size, const void *context);
static MC_TaskStatus terminate_task(MC_Task *task);

MC_Error mc_sched_new(MC_Sched **sched){
    MC_Sched *new = malloc(sizeof(MC_Sched));
    *sched = new;
    if(new == NULL){
        return MCE_OUT_OF_MEMORY;
    }

    memset(new, 0, sizeof(MC_Sched));
    return MCE_OK;
}

void mc_sched_delete(MC_Sched *sched){
    if(sched == NULL){
        return;
    }

    sched->flags |= SCHED_TERMINATING;
    while (sched->active_tasks || sched->finished){
        for(TaskNode *task = sched->active_tasks; task; task = task->next){
            if(task->task.flags & TASK_HARD_TERMINATE){
                task->task.do_some = terminate_task;
            }
        }

        mc_sched_continue(sched);
    }

    while(sched->garbage){
        TaskNode *node = sched->garbage;
        sched->garbage = sched->garbage->next;
        free(node);
    }

    free(sched);
}

MC_TaskStatus mc_sched_continue(MC_Sched *sched){
    MC_TaskStatus res = MC_TASK_SUSPEND;

    while(sched->finished){
        TaskNode *task = sched->finished;
        sched->finished = task->next;
        task->next = sched->garbage;
        sched->garbage = task;
    }


    for(TaskNode *task = sched->active_tasks, *prev = NULL, *next; task;){
        switch (task->task.do_some((MC_Task*)&task->task)){
        case MC_TASK_DONE:
            next = task->next;
            *(prev ? &prev->next : &sched->active_tasks) = task->pending ? task->pending : task->next;
            task->next = sched->finished;
            sched->finished = task;
            task = next;
            break;
        case MC_TASK_CONTINUE:
            res = MC_TASK_CONTINUE;
            // FALLTHROUGH
        case MC_TASK_SUSPEND:
            prev = task;
            task = task->next;
            break;
        }
    }

    if(sched->active_tasks) return res;
    else if(sched->finished) return MC_TASK_CONTINUE;
    else return MC_TASK_DONE;
}

void mc_sched_run(MC_Sched *sched, unsigned suspend_ms){
    while (true) switch (mc_sched_continue(sched)){
    case MC_TASK_DONE:
        return;
    case MC_TASK_SUSPEND:
        // TODO: sleep(suspend_ms);
        (void)suspend_ms;
        // FALLTHROUGH
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
        *task = (MC_Task*)&new->task;
        if(sched->active_tasks == NULL){
            sched->active_tasks = sched->active_last = new;
        }
        else{
            sched->active_last->next = new;
            sched->active_last = new;
        }
        return MCE_OK;
    }
}

MC_Error mc_run_task_after(MC_Task *prev, MC_Task **task, MC_TaskStatus (*do_some)(MC_Task *this), unsigned context_size, const void *context){
    MC_Sched *sched = prev->hdr.scheduler;
    TaskNode *prev_node = TASK_NODE(prev);
    if(sched->flags & SCHED_TERMINATING){
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

static MC_TaskStatus terminate_task(MC_Task *task){
    (void)task;
    return MC_TASK_DONE;
}