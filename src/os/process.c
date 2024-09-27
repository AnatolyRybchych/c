#include <mc/os/process.h>

#include <mc/data/string.h>
#include <mc/util/memory.h>

#include <errno.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>

#ifdef __linux__
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/signal.h>
    #include <sys/wait.h>

extern char **environ;
#endif


struct MC_Process{
#ifdef __linux__
    pid_t pid;
    bool finished;
    int exit_code;
#endif
};

static void *die_if_null(void *ptr);
static void *die_if_null(void *ptr);

MC_Error mc_process_run(MC_Process **ret_process, const MC_ProcessJob *job){
    MC_RETURN_INVALID(job->type >= __PROCESS_TYPES_CNT);

#ifdef __linux__
    pid_t pid = fork();
    if(pid < 0){
        return mc_error_from_errno(errno);
    }

    if(pid != 0){
        MC_RETURN_ERROR(mc_alloc(NULL, sizeof(MC_Process), (void**)ret_process));
        **ret_process = (MC_Process){
            .pid = pid,
        };
        return MCE_OK;
    }

    if(job->type == MC_PROCCESS_FORK){
        exit(job->as.fork.cb(job->as.fork.ctx));
    }

    size_t args_cnt = job->as.program.args_cnt;
    const MC_Str *args = job->as.program.args;

    char **argv = die_if_null(malloc(sizeof(char*[args_cnt+2])));
    MC_String *str;
    if(mc_string(NULL, &str, job->as.program.absolute_path)) exit(1);
    argv[0] = str->data;

    for(size_t i = 0; i < job->as.program.args_cnt; i++){
        if(mc_string(NULL, &str, args[i])) exit(1);
        argv[i + 1] = str->data;
    }

    argv[args_cnt + 1] = 0;
    *environ = NULL;
    exit(execvp(argv[0], argv));
#else
    return MCE_NOT_SUPPORTED;
#endif
}

void mc_process_free(MC_Process *process){
    mc_process_kill(process);
    mc_free(NULL, process);
}

MC_Error mc_process_program(MC_Process **process, MC_Str absolute_path, size_t args_cnt, const MC_Str args[]){
    MC_ProcessJob job = {
        .type = MC_PROCCESS_PROGRAM,
        .as.program = {
            .absolute_path = absolute_path,
            .args_cnt = args_cnt,
            .args = args
        }
    };

    return mc_process_run(process, &job);
}

MC_Error mc_process_fork(MC_Process **process, int (*cb)(void *ctx), void *ctx){
    MC_ProcessJob job = {
        .type = MC_PROCCESS_FORK,
        .as.fork = {
            .cb = cb,
            .ctx = ctx
        }
    };

    return mc_process_run(process, &job);
}

MC_Error mc_process_wait(MC_Process *process){
    if(process->finished){
        return MCE_OK;
    }

    waitpid(process->pid, &process->exit_code, 0);
    return MCE_OK;
}

MC_Error mc_process_kill(MC_Process *process){
    if(process->finished){
        return MCE_OK;
    }

    if(kill(process->pid, SIGKILL)){
        return mc_error_from_errno(process->pid);
    }

    process->finished = true;
    waitpid(process->pid, &process->exit_code, 0);
    return MCE_OK;
}

MC_Error mc_process_terminate(MC_Process *process){
    if(process->finished){
        return MCE_OK;
    }

    return kill(process->pid, SIGTERM)
        ? mc_error_from_errno(errno)
        : MCE_OK;
}

static void *die_if_null(void *ptr){
    if(ptr == NULL){
        exit(1);
    }

    return ptr;
}
