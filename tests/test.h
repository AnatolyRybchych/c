#ifndef TEST_H
#define TEST_H

#include <mc/time.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define RED  "\x1B[31m"
#define GRN  "\x1B[32m"
#define YEL  "\x1B[33m"
#define BLU  "\x1B[34m"
#define MAG  "\x1B[35m"
#define CYN  "\x1B[36m"
#define WHT  "\x1B[37m"
#define RST  "\x1B[0m"

#define TEST_ASSERTATION_MAX 100

#define TEST_FUNC_NAME(NAME) test_##NAME
#define TEST(NAME, ...) void TEST_FUNC_NAME(NAME)(TestContext *test, ##__VA_ARGS__)
#define ASSERT(...) (*test->assertation++ = (TestAssertation){ \
    .expr = #__VA_ARGS__, \
    .file = __FILE__, \
    .line = __LINE__, \
    .succeded = (__VA_ARGS__) \
})

typedef struct TestContext TestContext;
typedef struct TestAssertation TestAssertation;

TEST(time_diff_equ, MC_Time t1, MC_Time t2);
TEST(time_diff_lt, MC_Time t1, MC_Time t2);
TEST(time_diff_gt, MC_Time t1, MC_Time t2);
TEST(time_sum);
TEST(time_sum_overflow, MC_Time t1, MC_Time t2);
TEST(time_sum_no_overflow, MC_Time t1, MC_Time t2);

#define RUN_TESTS() \
    RUN_TEST(time_diff_equ, (MC_Time){.sec = 0, .nsec = 0}, (MC_Time){.sec = 0, .nsec = 0}) \
    RUN_TEST(time_diff_lt, (MC_Time){.sec = 0, .nsec = 0}, (MC_Time){.sec = 1, .nsec = 0}) \
    RUN_TEST(time_diff_lt, (MC_Time){.sec = 0, .nsec = 0}, (MC_Time){.sec = 0, .nsec = 1}) \
    RUN_TEST(time_diff_gt, (MC_Time){.sec = 1, .nsec = 0}, (MC_Time){.sec = 0, .nsec = 0}) \
    RUN_TEST(time_diff_gt, (MC_Time){.sec = 0, .nsec = 1}, (MC_Time){.sec = 0, .nsec = 0}) \
    RUN_TEST(time_sum) \
    RUN_TEST(time_sum_overflow, (MC_Time){.sec = UINT64_MAX, .nsec = MC_NSEC_IN_SEC}, (MC_Time){.sec = 0, .nsec = 1}) \
    RUN_TEST(time_sum_overflow, (MC_Time){.sec = 0, .nsec = 1}, (MC_Time){.sec = UINT64_MAX, .nsec = MC_NSEC_IN_SEC}) \
    RUN_TEST(time_sum_no_overflow, (MC_Time){.sec = UINT64_MAX - 1, .nsec = MC_NSEC_IN_SEC}, (MC_Time){.sec = 0, .nsec = 1}) \
    RUN_TEST(time_sum_no_overflow, (MC_Time){.sec = 0, .nsec = 1}, (MC_Time){.sec = UINT64_MAX - 1, .nsec = MC_NSEC_IN_SEC}) \


struct TestAssertation{
    const char *expr;
    const char *file;
    int line;
    bool succeded;
};

struct TestContext{
    bool succeeded;
    const char *test_name;

    TestAssertation *assertation;
    TestAssertation assertations[TEST_ASSERTATION_MAX];
};


#endif // TEST_H
