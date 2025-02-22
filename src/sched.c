#include <mc/sched.h>
#include <mc/data/pqueue.h>
#include <mc/data/list.h>
#include <mc/util/assert.h>
#include <mc/util/error.h>

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
    TASK_SCHEDULED = 1 << 1,
    TASK_DELAYED = 1 << 2,
    TASK_TODELETE = 1 << 3,
    TASK_DONE = 1 << 4,
    TASK_NEW = 1 << 5,
};

struct TaskHeader{
    TaskFlags flags;
    size_t ref_count;
    MC_Sched *scheduler;
    MC_Time scheduled_on;
    size_t dependencies;
    MC_TaskStatus (*do_some)(MC_Task *task);
};

struct MC_Task{
    TaskHeader hdr;
    size_t buffer_size;
    uint8_t buffer[];
};

struct TaskNode{
    TaskNode *next;
    TaskNode *subsequent;
    TaskNode *subsequent_next;
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

    size_t running_tasks;

    MC_PQueue *scheduled_tasks;
    TaskNode *active;
    TaskNode *active_back;
    TaskNode *finished;
    TaskNode *garbage;
};

struct RunAfterAllTaskData{
    TaskNode *task;
    size_t prev_cnt;
};

static TaskNode *create_task(MC_Sched *sched, MC_TaskStatus (*do_some)(MC_Task *this), unsigned context_size, const void *context);

static MC_Error schedule(MC_Sched *sched, TaskNode *task);
static void reschedule_active_tasks(MC_Sched *sched);
static void flush_finished_tasks(MC_Sched *sched);
static bool active_task_ready(TaskNode *task, const MC_Time *now);
static void activate_scheduled_tasks(MC_Sched *sched);
static TaskNode *pop_subsequent(TaskNode *task);
static void push_subsequent(TaskNode *task, TaskNode *subseq);
static void activate_subsequent_tasks(MC_Sched *sched, TaskNode *task);
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
    if(new == NULL){
        return MCE_OUT_OF_MEMORY;
    }

    memset(new, 0, sizeof(MC_Sched));
    new->now = now;

    new->scheduled_tasks = mc_pqueue_create(10, time_cmp);
    if(new->scheduled_tasks == NULL){
        free(new);
        return MCE_OUT_OF_MEMORY;
    }

    *sched = new;
    return MCE_OK;
}

void mc_sched_delete(MC_Sched *sched){
    if(sched == NULL){
        return;
    }

    sched->flags |= SCHED_TERMINATING;
    
    while(!mc_list_empty(sched->active)){
        MC_LIST_FOR(TaskNode, sched->active, active){
            if(active->task.flags & TASK_HARD_TERMINATE){
                active->task.do_some = terminate_task;
            }
        }

        mc_sched_continue(sched);
    }

    flush_finished_tasks(sched);

    for(TaskNode *garbage; (garbage = mc_list_remove(&sched->garbage));){
        free(garbage);
    }

    free(sched);
}

MC_TaskStatus mc_sched_continue(MC_Sched *sched){
    MC_TaskStatus res = MC_TASK_SUSPEND;

    flush_finished_tasks(sched);

    mc_gettime(MC_GETTIME_SINCE_BOOT, &sched->now);

    // apply mc_task_delay
    reschedule_active_tasks(sched);

    activate_scheduled_tasks(sched);

    for(TaskNode *task; (task = mc_list_remove(&sched->active));){
        if(!active_task_ready(task, &sched->now)){
            mc_list_add(sched->active_back, task);
            continue;
        }

        switch (do_some(task)){
        case MC_TASK_DONE:
            activate_subsequent_tasks(sched, task);
            task->task.flags |= TASK_DONE;
            mc_list_add(&sched->finished, task);
            break;
        case MC_TASK_CONTINUE:
            res = MC_TASK_CONTINUE;
            // FALLTHROUGH
        case MC_TASK_SUSPEND:
            mc_list_add(&sched->active_back, task);
            break;
        }

        break;
    }

    if(mc_list_empty(sched->active)) {
        mc_list_add(&sched->active, sched->active_back);
        sched->active_back = NULL;
    }

    if(!mc_list_empty(sched->active)) return res;
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

void mc_sched_set_suspend_interval(MC_Sched *sched, MC_Time suspend){
    sched->suspend = suspend;
}

MC_Error mc_task_run(MC_Task *task) {
    return mc_task_run_aftern(task, 0, NULL);
}

MC_Error mc_task_run_aftern(MC_Task *task, size_t tasks_cnt, MC_Task **tasks) {
    MC_RETURN_INVALID(task == NULL);

    MC_Sched *sched = mc_task_get_sched(task);
    TaskNode *node = TASK_NODE(task);

    if(tasks_cnt == 0) {
        mc_list_add(&sched->active, node);
    }
    else{
        node->task.dependencies = tasks_cnt;
        for(MC_Task **dependency_ptr = tasks; dependency_ptr != &tasks[tasks_cnt]; dependency_ptr++) {
            TaskNode *dependency_node = TASK_NODE(*dependency_ptr);
            push_subsequent(dependency_node, node);
        }
    }

    task->hdr.flags &= ~TASK_NEW;
    return MCE_OK;
}

MC_Error mc_task_run_afterv(MC_Task *task, MC_Task *dependency, va_list dependencies) {
    MC_Task *dependencies_arr[32] = {dependency};
    size_t dependencies_cnt = 1;

    for(MC_Task *cur; (cur = va_arg(dependencies, MC_Task*));) {
        if(++dependencies_cnt > sizeof dependencies_arr / sizeof * dependencies_arr) {
            return MCE_INVALID_INPUT;
        }

        dependencies_arr[dependencies_cnt-1] = cur;
    }

    return mc_task_run_aftern( task, dependencies_cnt, dependencies_arr);
}

MC_Error mc_task_run_after(MC_Task *task, MC_Task *dependency, ...) {
    va_list args;
    va_start(args, dependency);
    MC_Error error = mc_task_run_afterv(task, dependency, args);
    va_end(args);
    return error;
}

MC_Error mc_task_sched(MC_Task *task, MC_Time timeout) {
    MC_RETURN_INVALID(task == NULL);
    MC_RETURN_INVALID(!(task->hdr.flags & TASK_NEW));

    MC_Sched *sched = mc_task_get_sched(task);

    MC_Time scheduled_on;
    MC_RETURN_ERROR(mc_timesum(&sched->now, &timeout, &scheduled_on));
    MC_RETURN_ERROR(schedule(sched, TASK_NODE(task)));

    task->hdr.flags &= ~TASK_NEW;
    return MCE_OK;
}

MC_Error mc_task_new(MC_Sched *sched, MC_Task **task, MC_TaskStatus (*do_some)(MC_Task *this), unsigned context_size, const void *context) {
    if(sched->flags & SCHED_TERMINATING){
        *task = NULL;
        return MCE_BUSY;
    }

    TaskNode *new = create_task(sched, do_some, context_size, context);
    if(new == NULL){
        *task = NULL;
        return MCE_OUT_OF_MEMORY;
    }

    *task = TASK(new);
    return MCE_OK;
}

void *mc_task_data(MC_Task *task, unsigned *size){
    if(size){
        *size = task->buffer_size;
    }

    return task->buffer;
}

MC_Sched *mc_task_get_sched(MC_Task *task){
    return task->hdr.scheduler;
}

MC_Error mc_task_delay(MC_Task *task, MC_Time delay){
    if(task->hdr.flags & TASK_SCHEDULED){
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

void mc_task_set_handler(MC_Task *task, MC_TaskStatus (*do_some)(MC_Task *this)){
    task->hdr.do_some = do_some ? do_some : terminate_task;
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

    if(task->hdr.flags & (TASK_TODELETE | TASK_NEW)){
        free(task);
    }
}

MC_Error mc_task_wait(const MC_Time *timeout, MC_Task *task, ...){
    MC_Sched *sched = task->hdr.scheduler;

    MC_Time wait_until;
    if(timeout && mc_timesum(&sched->now, timeout, &wait_until) == MCE_OVERFLOW){
        wait_until = (MC_Time){.sec = ~0LLU};
    }

    va_list tasks;
    va_start(tasks, task);

    for(MC_Task *cur = task; cur; cur = va_arg(tasks, MC_Task*)){
        while (!(cur->hdr.flags & TASK_DONE)){
            switch (mc_sched_continue(sched)){
            case MC_TASK_DONE:
                break;
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

const char *mc_task_status_str(MC_TaskStatus status){
    const char *result = NULL;
    MC_ENUM_TO_STRING(result, status, MC_ITER_TASK_STATUS, MC_TASK_)
    return result;
}

static TaskNode *create_task(MC_Sched *sched, MC_TaskStatus (*do_some)(MC_Task *this), unsigned context_size, const void *context){
    TaskNode *new;

    if(!mc_list_empty(sched->garbage)){
        TaskNode *garbage = mc_list_remove(&sched->garbage);
        if(context_size > garbage->buffer_capacity){
            new = realloc(garbage, sizeof(TaskNode) + context_size);
            if(new == NULL){
                free(garbage);
                return NULL;
            }

            new->buffer_capacity = context_size;
        }
        else{
            free(garbage);
            return NULL;
        }
    }
    else{
        new = malloc(sizeof(TaskNode) + context_size);
        if(new == NULL){
            return NULL;
        }

        new->buffer_capacity = context_size;
    }

    *new = (TaskNode){0};

    new->task.ref_count = 1;
    new->task.scheduler = sched;
    new->task.do_some = do_some;
    new->buffer_size = context_size;
    new->task.flags |= TASK_NEW;;

    if(context)
        memcpy(new->buffer, context, context_size);

    return new;
}


static MC_Error schedule(MC_Sched *sched, TaskNode *task){
    MC_PQueue *scheduled_tasks = mc_pqueue_enqueue(sched->scheduled_tasks, task);
    task->task.flags &= ~TASK_DELAYED;
    task->task.flags |= TASK_SCHEDULED;
    if(!scheduled_tasks){
        return MCE_OUT_OF_MEMORY;
    }
    
    sched->scheduled_tasks = scheduled_tasks;
    return MCE_OK;
}

static bool active_task_ready(TaskNode *task, const MC_Time *now){
    if(task->task.flags & TASK_SCHEDULED){
        return mc_timecmp(now, &task->task.scheduled_on) >= 0;
    }

    return true;
}

static void activate_scheduled_tasks(MC_Sched *sched){
    for(TaskNode *scheduled = mc_pqueue_getv(sched->scheduled_tasks);
    scheduled && mc_timecmp(&sched->now, &scheduled->task.scheduled_on) >= 0;
    scheduled = mc_pqueue_getv(sched->scheduled_tasks)){
        TaskNode *active = mc_pqueue_dequeuev(sched->scheduled_tasks);
        active->task.flags &= ~(TASK_SCHEDULED | TASK_DELAYED);
        mc_list_add(&sched->active, active);
    }
}

static TaskNode *pop_subsequent(TaskNode *task){
    TaskNode *result = task->subsequent;
    if(task->subsequent) {
        task->subsequent = task->subsequent->subsequent_next;
    }

    return result;
}

static void push_subsequent(TaskNode *task, TaskNode *subseq){
    // only one at the time to avoid huge iterations
    MC_ASSERT_BUG(subseq->subsequent_next == NULL);

    if(!task->subsequent) {
        task->subsequent = subseq;
        return;
    }

    subseq->subsequent_next = task->subsequent;
    task->subsequent = subseq;
}

static void activate_subsequent_tasks(MC_Sched *sched, TaskNode *task) {
    for(TaskNode *subseq; (subseq = pop_subsequent(task));) {
        MC_ASSERT_BUG(subseq->task.dependencies != 0);
        if(--subseq->task.dependencies == 0) {
            mc_list_add(&sched->active_back, subseq);
        }
    }
}

static void flush_finished_tasks(MC_Sched *sched){
    for(TaskNode *finished; (finished = mc_list_remove(&sched->finished));) {
        if(finished->task.ref_count == 0){
            mc_list_add(&sched->garbage, finished);
        }
        else{
            finished->task.flags |= TASK_TODELETE;
        }
    }
}

static void reschedule_active_tasks(MC_Sched *sched){
    for(TaskNode **active_location = &sched->active; *active_location;) {
        TaskNode *active = *active_location;
        if(active->task.flags & (TASK_DELAYED | TASK_SCHEDULED)){
            active->task.flags &= ~(TASK_DELAYED | TASK_SCHEDULED);

            mc_list_remove(active_location);
            if(schedule(sched, active) != MCE_OK){
                mc_list_add(&active_location, active);
                active_location = &active->next;
            }
        }
        else {
            active_location = &active->next;
        }
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
    return task->task.do_some(TASK(task));
}