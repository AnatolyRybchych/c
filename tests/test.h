#ifndef TEST_H
#define TEST_H

#include <mc/time.h>
#include <mc/net/address.h>

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

TEST(address_to_string, MC_Address addr, const char *res);

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
    \
    RUN_TEST(address_to_string, (MC_Address){.type = MC_ADDRTYPE_ETHERNET, .ether.data = {0, 0, 0, 0, 0, 0}}, "00:00:00:00:00:00") \
    RUN_TEST(address_to_string, (MC_Address){.type = MC_ADDRTYPE_ETHERNET, .ether.data = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}}, "AA:BB:CC:DD:EE:FF") \
    RUN_TEST(address_to_string, (MC_Address){.type = MC_ADDRTYPE_IPV4, .ipv4.data = {192, 168, 1, 1}}, "192.168.1.1") \
    RUN_TEST(address_to_string, (MC_Address){.type = MC_ADDRTYPE_IPV6, .ipv6.data = {0x20, 0x00}}, "2000::") \
    RUN_TEST(address_to_string, (MC_Address){.type = MC_ADDRTYPE_IPV6, .ipv6.data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}}, "0102:0304:0506:0708:090a:0b0c:0d0e:0f10") \
    RUN_TEST(address_to_string, (MC_Address){.type = MC_ADDRTYPE_IPV6, .ipv6.data = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 192, 168, 1, 1}}, "::ffff:192.168.1.1") \


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
