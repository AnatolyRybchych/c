#include "test.h"

TEST(time_diff_equ, MC_Time t1, MC_Time t2){
    ASSERT(mc_timecmp(&t1, &t2) == MC_SIGN_EQUALS);
}

TEST(time_diff_lt, MC_Time t1, MC_Time t2){
    ASSERT(mc_timecmp(&t1, &t2) == MC_SIGN_LESS);
}

TEST(time_diff_gt, MC_Time t1, MC_Time t2){
    ASSERT(mc_timecmp(&t1, &t2) == MC_SIGN_GREATER);
}
