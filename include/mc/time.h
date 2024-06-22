#ifndef MC_TIME_H
#define MC_TIME_H

#include <stdint.h>
#include <mc/error.h>
#include <mc/util/sign.h>

typedef struct MC_Time MC_Time;

typedef unsigned MC_GetTime;
enum MC_GetTime{
    MC_GETTIME_UTC_SINCE_EPOCH,
    MC_GETTIME_LOCAL_SINCE_EPOCH,
    MC_GETTIME_SINCE_FIRSTBOOT,
    MC_GETTIME_SINCE_BOOT,
    MC_GETTIME_SINCE_PROCCESS_START,
};

MC_Error mc_gettime(MC_GetTime gettime, MC_Time *time);
MC_Sign mc_timecmp(const MC_Time *time1, const MC_Time *time2);
MC_Sign mc_timediff(const MC_Time *time1, const MC_Time *time2, MC_Time *diff);

struct MC_Time{
    uint64_t sec;
    uint64_t nsec;
};

#endif // MC_TIME_H
