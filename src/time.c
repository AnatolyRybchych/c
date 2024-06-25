#include <mc/time.h>
#include <mc/util/assert.h>

#ifdef __linux__
#include <time.h>
#include <sys/sysinfo.h>
#endif

MC_Error mc_gettime(MC_GetTime gettime, MC_Time *time){
    switch (gettime){
#ifdef __linux__
    case MC_GETTIME_SINCE_BOOT:{
        struct timespec ts;
        if(clock_gettime(CLOCK_BOOTTIME, &ts))
            return MCE_NOT_SUPPORTED;

        *time = (MC_Time){
            .sec = ts.tv_sec,
            .nsec = ts.tv_nsec,
        };
    } return MCE_OK;
    case MC_GETTIME_SINCE_PROCCESS_START:{
        struct timespec ts;
        if(clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts))
            return MCE_NOT_SUPPORTED;

        *time = (MC_Time){
            .sec = ts.tv_sec,
            .nsec = ts.tv_nsec,
        };
    } return MCE_OK;
#endif
    default:
        *time = (MC_Time){0};
        return MCE_NOT_IMPLEMENTED;
    }
}

MC_Sign mc_timecmp(const MC_Time *time1, const MC_Time *time2){
    if(time1->sec != time2->sec)
        return time1->sec > time2->sec ? MC_SIGN_GREATER : MC_SIGN_LESS;
    else if(time1->nsec == time2->nsec) return MC_SIGN_EQUALS;
    else return time1->nsec > time2->nsec ? MC_SIGN_GREATER : MC_SIGN_LESS;
}

MC_Sign mc_timediff(const MC_Time *time1, const MC_Time *time2, MC_Time *diff){
    switch (mc_timecmp(time1, time2)){
    case MC_SIGN_LESS:
        if((int64_t)time2->nsec - (int64_t)time1->nsec < 0){
            *diff = (MC_Time){
                .sec = time2->sec - time1->sec - 1,
                .nsec = 1000000000 + (int64_t)time2->nsec - (int64_t)time1->nsec
            };
        }
        else{
            *diff = (MC_Time){
                .sec = time2->sec - time1->sec,
                .nsec = time2->nsec - time1->nsec
            };
        }
        return MC_SIGN_EQUALS;
    case MC_SIGN_GREATER:
        if((int64_t)time1->nsec - (int64_t)time2->nsec < 0){
            *diff = (MC_Time){
                .sec = time1->sec - time2->sec - 1,
                .nsec = 1000000000 + (int64_t)time1->nsec - (int64_t)time2->nsec
            };
        }
        else{
            *diff = (MC_Time){
                .sec = time1->sec - time2->sec,
                .nsec = time1->nsec - time2->nsec
            };
        }
        return MC_SIGN_EQUALS;
    default:
        *diff = (MC_Time){0};
        return MC_SIGN_EQUALS;
    }
}

MC_Error mc_timesum(const MC_Time *time1, const MC_Time *time2, MC_Time *result){
    MC_Error status = MCE_OK;

    uint64_t sum_nsec = (time1->nsec % MC_NSEC_IN_SEC)
        + time2->nsec % MC_NSEC_IN_SEC;

    uint64_t sum_sec = (time1->nsec / MC_NSEC_IN_SEC)
        + (time2->nsec / MC_NSEC_IN_SEC)
        + (sum_nsec / MC_NSEC_IN_SEC);

    sum_nsec %= MC_NSEC_IN_SEC;

    MC_Time sum = {
        .sec = sum_sec,
        .nsec = sum_nsec
    };

    sum.sec += time1->sec;
    if(sum.sec < sum_sec){
        status = MCE_OVERFLOW;
    }

    sum_sec = sum.sec;
    sum.sec += time2->sec;
    if(sum.sec < sum_sec){
        status = MCE_OVERFLOW;
    }

    *result = sum;

    return status;
}

MC_Error mc_sleep(const MC_Time *time){
    struct timespec delay = {
        .tv_sec = time->sec,
        .tv_nsec = time->nsec
    };

    if (nanosleep(&delay, NULL) < 0) {
        return MCE_NOT_SUPPORTED;
    }

    return MCE_OK;
}
