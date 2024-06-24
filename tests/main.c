#include "test.h"

static void begin_test(TestContext *ctx, const char *test_name, const char *args);
static void finish_test(TestContext *ctx);

int main(int argc, char *argv[]){
    (void)argc;
    (void)argv;

    TestContext ctx;

    #define RUN_TEST(TEST_NAME, ...) \
        begin_test(&ctx, #TEST_NAME, #__VA_ARGS__); \
        TEST_FUNC_NAME(TEST_NAME)(&ctx, ##__VA_ARGS__); \
        finish_test(&ctx);

    RUN_TESTS()

    #undef RUN_TEST
}

static void begin_test(TestContext *ctx, const char *test_name, const char *args){
    printf("TEST %s(%s)\n", test_name, args);

    *ctx = (TestContext){
        .succeeded = true,
        .test_name = test_name,
        .assertation = ctx->assertations,
    };
}

static void finish_test(TestContext *ctx){
    for(TestAssertation *a = ctx->assertations; a != ctx->assertation; a++){
        if(a->succeded){
            printf(GRN "    %s -> OK\n" RST, a->expr);
        }
        else{
            printf(RED "    %s -> FAIL\n" RST, a->expr);
            printf(RED "%s: %i\n" RST, a->file, a->line);
        }
    }

    if(!ctx->succeeded){
        printf(RED "    ERROR\n" RST);
        exit(1);
    }
}
