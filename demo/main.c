#include <stdio.h>

#include <mc/dlib.h>
#include <mc/sched.h>
#include <mc/time.h>
#include <mc/data/pqueue.h>
#include <mc/data/list.h>
#include <mc/io/stream.h>
#include <mc/io/file.h>

#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

int main(){
    mc_stream_write_all(MC_STDOUT, 6, "test\n");
    mc_stream_flush(MC_STDOUT);
}

