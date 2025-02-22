#include <mc/os/file.h>
#include <mc/util/assert.h>
#include <mc/time.h>
#include <mc/sched.h>

#define L(...) (mc_fmt(MC_STDOUT, "%s\n", #__VA_ARGS__), __VA_ARGS__)
#define LR(...) MC_REQUIRE(L(__VA_ARGS__))
#define DELIM() mc_fmt(MC_STDOUT, "================================\n\n");
#define NL() mc_fmt(MC_STDOUT, "\n");

typedef struct {
    char name[64];
    int iterations;
    bool suspend;
} TaskData;

MC_TaskStatus do_some(MC_Task *this){
    TaskData *data = mc_task_data(this, NULL);

    mc_fmt(MC_STDOUT, "TASK %s; iterations: %d, suspend: %s\n",
        data->name, data->iterations, data->suspend ? "true" : "false");

    if(--data->iterations <= 0) {
        return L(MC_TASK_DONE);
    }

    if(data->suspend) {
        return L(MC_TASK_SUSPEND);
    }
    else{
        return L(MC_TASK_CONTINUE);
    }
}

int main(){
    MC_Sched *sched;
    MC_REQUIRE(mc_sched_new(&sched));

    MC_Task *root;
    MC_Task *subseq1, *subseq2;
    MC_Task *subseq1_1, *subseq1_2, *subseq1_3;
    MC_Task *subseq2_1, *subseq2_2, *subseq2_3;
    MC_Task *tail;

    LR(mc_task_new(sched, &root, do_some, sizeof(TaskData), &(TaskData){
        .name = "ROOT",
        .iterations = 3
    }));

    LR(mc_task_new(sched, &subseq1, do_some, sizeof(TaskData), &(TaskData){
        .name = "ROOT -> subsequent 1",
        .iterations = 3
    }));
    LR(mc_task_new(sched, &subseq2, do_some, sizeof(TaskData), &(TaskData){
        .name = "ROOT -> subsequent 2",
        .iterations = 1
    }));

    LR(mc_task_new(sched, &subseq1_1, do_some, sizeof(TaskData), &(TaskData){
        .name = "ROOT -> subsequent 1 -> subsequent 1_1",
        .iterations = 3
    }));
    LR(mc_task_new(sched, &subseq1_2, do_some, sizeof(TaskData), &(TaskData){
        .name = "ROOT -> subsequent 1 -> subsequent 1_2",
        .iterations = 3
    }));
    LR(mc_task_new(sched, &subseq1_3, do_some, sizeof(TaskData), &(TaskData){
        .name = "ROOT -> subsequent 1 -> subsequent 1_3",
        .iterations = 3
    }));

    LR(mc_task_new(sched, &subseq2_1, do_some, sizeof(TaskData), &(TaskData){
        .name = "ROOT -> subsequent 2 -> subsequent 2_1",
        .iterations = 3
    }));
    LR(mc_task_new(sched, &subseq2_2, do_some, sizeof(TaskData), &(TaskData){
        .name = "ROOT -> subsequent 2 -> subsequent 2_2",
        .iterations = 3
    }));
    LR(mc_task_new(sched, &subseq2_3, do_some, sizeof(TaskData), &(TaskData){
        .name = "ROOT -> subsequent 2 -> subsequent 2_3",
        .iterations = 3
    }));

    LR(mc_task_new(sched, &tail, do_some, sizeof(TaskData), &(TaskData){
        .name = "LAST",
        .iterations = 3
    }));

    DELIM();
    NL();

    LR(mc_task_run(root));
    L(mc_task_unref(root));
    DELIM();

    LR(mc_task_run_after(subseq1, root, NULL));
    L(mc_task_unref(subseq1));
    LR(mc_task_run_after(subseq2, root, NULL));
    L(mc_task_unref(subseq2));
    DELIM();

    LR(mc_task_run_after(subseq1_1, subseq1, NULL));
    L(mc_task_unref(subseq1_1));
    LR(mc_task_run_after(subseq1_2, subseq1, NULL));
    L(mc_task_unref(subseq1_2));
    LR(mc_task_run_after(subseq1_3, subseq1, NULL));
    L(mc_task_unref(subseq1_3));
    DELIM();

    LR(mc_task_run_after(subseq2_1, subseq2, NULL));
    L(mc_task_unref(subseq2_1));
    LR(mc_task_run_after(subseq2_2, subseq2, NULL));
    L(mc_task_unref(subseq2_2));
    LR(mc_task_run_after(subseq2_3, subseq2, NULL));
    L(mc_task_unref(subseq2_3));
    DELIM();

    LR(mc_task_run_after(tail, subseq1_1, subseq1_2, subseq1_3, subseq2_1, subseq2_2, subseq2_3, NULL));
    L(mc_task_unref(tail));
    DELIM();

    L(mc_sched_run(sched));
}
