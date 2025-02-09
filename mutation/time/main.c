#include <mc/os/file.h>
#include <mc/util/assert.h>
#include <mc/time.h>

#define L(...) (mc_fmt(MC_STDOUT, "%s\n", #__VA_ARGS__), __VA_ARGS__)
#define LR(...) MC_REQUIRE(L(__VA_ARGS__))
#define DELIM() mc_fmt(MC_STDOUT, "================================\n\n");
#define NL() mc_fmt(MC_STDOUT, "\n");

int main(){
    int cmp_result; 
    L(cmp_result = mc_timecmp(&(MC_Time){.sec = 0, .nsec = 0}, &(MC_Time){.sec = 0, .nsec = 0}));
    L(mc_fmt(MC_STDOUT, "cmp_result: %d\n", cmp_result));
    DELIM();

    L(cmp_result = mc_timecmp(&(MC_Time){.sec = 0, .nsec = 0}, &(MC_Time){.sec = 1, .nsec = 0}));
    L(mc_fmt(MC_STDOUT, "cmp_result: %d\n", cmp_result));
    DELIM();

    L(cmp_result = mc_timecmp(&(MC_Time){.sec = 0, .nsec = 0}, &(MC_Time){.sec = 0, .nsec = 1}));
    L(mc_fmt(MC_STDOUT, "cmp_result: %d\n", cmp_result));
    DELIM();

    L(cmp_result = mc_timecmp(&(MC_Time){.sec = 1, .nsec = 0}, &(MC_Time){.sec = 0, .nsec = 0}));
    L(mc_fmt(MC_STDOUT, "cmp_result: %d\n", cmp_result));
    DELIM();

    L(cmp_result = mc_timecmp(&(MC_Time){.sec = 0, .nsec = 1}, &(MC_Time){.sec = 0, .nsec = 0}));
    L(mc_fmt(MC_STDOUT, "cmp_result: %d\n", cmp_result));
    DELIM();

    L(cmp_result = mc_timecmp(&(MC_Time){.sec = 0, .nsec = 1}, &(MC_Time){.sec = 0, .nsec = 0}));
    L(mc_fmt(MC_STDOUT, "cmp_result: %d\n", cmp_result));
    DELIM();

    MC_Time timesum_result;

    L(mc_timesum(&(MC_Time){.nsec = 10LLU * MC_NSEC_IN_SEC}, &(MC_Time){0}, &timesum_result));
    L(mc_fmt(MC_STDOUT, "sec: %llu, nsec: %llu\n", timesum_result.sec, timesum_result.nsec));
    DELIM();

    L(mc_timesum(&(MC_Time){.sec = UINT64_MAX}, &(MC_Time){.sec = 1}, &timesum_result));
    L(mc_fmt(MC_STDOUT, "sec: %llu, nsec: %llu\n", timesum_result.sec, timesum_result.nsec));
    DELIM();

    L(mc_timesum(&(MC_Time){.sec = UINT64_MAX}, &(MC_Time){.sec = 2}, &timesum_result));
    L(mc_fmt(MC_STDOUT, "sec: %llu, nsec: %llu\n", timesum_result.sec, timesum_result.nsec));
    DELIM();

    MC_Error timesum_error;
    L(timesum_error = mc_timesum(&(MC_Time){.sec = UINT64_MAX, .nsec = MC_NSEC_IN_SEC}, &(MC_Time){.sec = 0, .nsec = 1}, &timesum_result));
    L(mc_fmt(MC_STDOUT, "sec: %llu, nsec: %llu\n", timesum_result.sec, timesum_result.nsec));
    L(mc_fmt(MC_STDOUT, "timesum_error: %s; (expected to be OVERFLOW)\n", mc_strerror(timesum_error)));
    DELIM();

    L(timesum_error = mc_timesum(&(MC_Time){.sec = 0, .nsec = 1}, &(MC_Time){.sec = UINT64_MAX, .nsec = MC_NSEC_IN_SEC}, &timesum_result));
    L(mc_fmt(MC_STDOUT, "sec: %llu, nsec: %llu\n", timesum_result.sec, timesum_result.nsec));
    L(mc_fmt(MC_STDOUT, "timesum_error: %s; (expected to be OVERFLOW)\n", mc_strerror(timesum_error)));
    DELIM();

    L(timesum_error = mc_timesum(&(MC_Time){.sec = UINT64_MAX - 1, .nsec = MC_NSEC_IN_SEC}, &(MC_Time){.sec = 0, .nsec = 1}, &timesum_result));
    L(mc_fmt(MC_STDOUT, "sec: %llu, nsec: %llu\n", timesum_result.sec, timesum_result.nsec));
    L(mc_fmt(MC_STDOUT, "timesum_error: %s; (expected to be OK)\n", mc_strerror(timesum_error)));
    DELIM();

    L(timesum_error = mc_timesum(&(MC_Time){.sec = 0, .nsec = 1}, &(MC_Time){.sec = UINT64_MAX - 1, .nsec = MC_NSEC_IN_SEC}, &timesum_result));
    L(mc_fmt(MC_STDOUT, "sec: %llu, nsec: %llu\n", timesum_result.sec, timesum_result.nsec));
    L(mc_fmt(MC_STDOUT, "timesum_error: %s; (expected to be OK)\n", mc_strerror(timesum_error)));
    DELIM();
}
