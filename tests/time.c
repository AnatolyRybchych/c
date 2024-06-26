#include "test.h"

TEST(time_diff_equ, MC_Time t1, MC_Time t2){
    ASSERT(mc_timecmp(&t1, &t2) == 0);
}

TEST(time_diff_lt, MC_Time t1, MC_Time t2){
    ASSERT(mc_timecmp(&t1, &t2) < 0);
}

TEST(time_diff_gt, MC_Time t1, MC_Time t2){
    ASSERT(mc_timecmp(&t1, &t2) > 0);
}
