#include <stdio.h>

#include <mc/dlib.h>
#include <mc/sched.h>
#include <mc/time.h>
#include <mc/data/pqueue.h>
#include <mc/data/list.h>
#include <mc/data/string.h>
#include <mc/io/stream.h>
#include <mc/io/file.h>

#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

int main(){
    MC_String *test = mc_string_fmt("test %i, %zu\n", 10, (size_t)20);
    printf("%s\n", test->data);

    mc_stream_fmt(MC_STDOUT, "test %i, %zu\n", 10, (size_t)20);
    mc_stream_flush(MC_STDOUT);
}

