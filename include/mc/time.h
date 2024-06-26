#ifndef MC_TIME_H
#define MC_TIME_H

#include <stdint.h>
#include <mc/error.h>

#define MC_MSEC_IN_SEC 1000
#define MC_USEC_IN_SEC (MC_MSEC_IN_SEC * 1000)
#define MC_NSEC_IN_SEC (MC_USEC_IN_SEC * 1000)

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
int mc_timecmp(const MC_Time *time1, const MC_Time *time2);
int mc_timediff(const MC_Time *time1, const MC_Time *time2, MC_Time *diff);

/// @return OVERFLOW in case of overflow
MC_Error mc_timesum(const MC_Time *time1, const MC_Time *time2, MC_Time *result);
MC_Error mc_sleep(const MC_Time *time);

struct MC_Time{
    uint64_t sec;
    uint64_t nsec;
};

#endif // MC_TIME_H
