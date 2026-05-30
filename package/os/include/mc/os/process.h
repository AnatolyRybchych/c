#ifndef MC_OS_PROCESS_H
#define MC_OS_PROCESS_H

#include <mc/error.h>
#include <mc/data/str.h>

typedef struct MC_Process MC_Process;
typedef struct MC_ProcessJob MC_ProcessJob;
typedef unsigned MC_ProcessJobType;
enum MC_ProcessJobType{
    MC_PROCCESS_FORK,
    MC_PROCCESS_PROGRAM,

    __PROCESS_TYPES_CNT
};

MC_Error mc_process_run(MC_Process **process, const MC_ProcessJob *job);
void mc_process_free(MC_Process *process);

MC_Error mc_process_program(MC_Process **process, MC_Str absolute_path, size_t argc, const MC_Str argv[]);
MC_Error mc_process_fork(MC_Process **process, int (*cb)(void *ctx), void *ctx);

MC_Error mc_process_wait(MC_Process *process);
MC_Error mc_process_kill(MC_Process *process);
MC_Error mc_process_terminate(MC_Process *process);

struct MC_ProcessJob{
    MC_ProcessJobType type;
    union {
        struct{
            void *ctx;
            int (*cb)(void *ctx);
        } fork;

        struct{
            MC_Str absolute_path;
            size_t args_cnt;
            const MC_Str *args;
        } program;
    } as;
};

#endif // MC_OS_PROCESS_H
