#include <mc/util/assert.h>
#include <mc/util/minmax.h>
#include <mc/sched.h>
#include <mc/data/string.h>
#include <stdlib.h>

typedef struct ReadFileTask ReadFileTask;

struct ReadFileTask{
    size_t write_size;
    char write_buf[256];

    size_t max_size;
    MC_Stream *src;
    MC_Stream *dst;
    MC_Error error;
};

static MC_TaskStatus read_file(MC_Task *this) {
    ReadFileTask *data = mc_task_data(this, NULL);

    if(data->write_size) {
        size_t written;
        data->error = mc_write_async(data->dst, data->write_size, data->write_buf, &written);
        data->write_size -= written;

        return data->error == MCE_AGAIN
            ? MC_TASK_SUSPEND
            : MC_TASK_DONE;
    }

    size_t read;
    MC_Error read_error = mc_read_async(data->src, MIN(sizeof(data->write_buf), data->max_size), data->write_buf, &read);
    data->write_size = read;
    data->max_size -= read;

    return data->max_size == 0 || (read == 0 && read_error != MCE_AGAIN)
        ? MC_TASK_DONE
        : MC_TASK_SUSPEND;
}

int main(){
    MC_Sched *sched = NULL;
    mc_sched_new(&sched);

    MC_Task *read_file_task = NULL;

    MC_Stream *random, *dst;
    MC_REQUIRE(mc_fopen(&random, MC_STRC("/dev/random"), MC_OPEN_READ | MC_OPEN_ASYNC));
    MC_REQUIRE(mc_fopen(&dst, MC_STRC("./random_data"), MC_OPEN_CREATE | MC_OPEN_READ | MC_OPEN_WRITE | MC_OPEN_ASYNC));

    mc_run_task(sched, &read_file_task, read_file, sizeof(ReadFileTask), &(ReadFileTask){
        .max_size = 10000,
        .src = random,
        .dst = dst
    });

    mc_task_wait(NULL, read_file_task, NULL);
    mc_close(random);
    mc_close(dst);

    mc_sched_run(sched);
    mc_sched_delete(sched);
}

