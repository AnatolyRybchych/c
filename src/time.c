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
    if(time2->sec != time1->sec)
        return time2->sec > time1->sec ? MC_SIGN_GREATER : MC_SIGN_LESS;
    else if(time2->nsec == time1->nsec) return MC_SIGN_EQUALS;
    else return time2->nsec > time1->nsec ? MC_SIGN_GREATER : MC_SIGN_LESS;
}

MC_Sign mc_timediff(const MC_Time *time1, const MC_Time *time2, MC_Time *diff){
    switch (mc_timecmp(time1, time2)){
    case MC_SIGN_LESS:
        *diff = (MC_Time){.sec = time1->sec - time2->sec, .nsec = time1->nsec - time2->nsec};
        return MC_SIGN_EQUALS;
    case MC_SIGN_GREATER:
        *diff = (MC_Time){.sec = time2->sec - time1->sec, .nsec = time2->nsec - time1->nsec};
        return MC_SIGN_EQUALS;
    default:
        *diff = (MC_Time){0};
        return MC_SIGN_EQUALS;
    }
}
