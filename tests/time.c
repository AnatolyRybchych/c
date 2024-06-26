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

TEST(time_sum){
    MC_Time result;

    ASSERT(mc_timesum(&(MC_Time){.nsec = 10LLU * MC_NSEC_IN_SEC}, &(MC_Time){0}, &result),
        result.sec == 10 && result.nsec == 0);

    ASSERT(mc_timesum(&(MC_Time){.sec = UINT64_MAX}, &(MC_Time){.sec = 1}, &result),
        result.sec == 0 && result.nsec == 0);
    
    ASSERT(mc_timesum(&(MC_Time){.sec = UINT64_MAX}, &(MC_Time){.sec = 2}, &result),
        result.sec == 1 && result.nsec == 0);
}

TEST(time_sum_overflow, MC_Time t1, MC_Time t2){
    MC_Time result;
    ASSERT(mc_timesum(&t1, &t2, &result) == MCE_OVERFLOW);
}

TEST(time_sum_no_overflow, MC_Time t1, MC_Time t2){
    MC_Time result;
    ASSERT(mc_timesum(&t1, &t2, &result) != MCE_OVERFLOW);
}
