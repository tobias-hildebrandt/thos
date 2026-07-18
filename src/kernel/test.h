#pragma once

#include <assert.h>  // IWYU pragma: keep
#include <stdio.h>   // IWYU pragma: keep
#include <stdlib.h>  // IWYU pragma: keep

#include "device/device_tree.h"
#include "util.h"

#define TEST_NAME_PREFIX _test_
enum {
    TEST_NAME_PREFIX_LEN =
        CHARS_IN_STRING_LITERAL(STRINGIFY_VALUE(TEST_NAME_PREFIX))
};
#define TEST_NAME_SHOULD_FAIL_SUFFIX should_fail

#define TEST_NAME_(PREFIX, NAME) CONCAT_(PREFIX, NAME)
#define TEST_NAME(NAME) TEST_NAME_(TEST_NAME_PREFIX, NAME)

#define TEST_NAME_SHOULD_FAIL_(PREFIX, NAME, SUFFIX) \
    CONCAT3_(_test_, NAME, should_fail)
#define TEST_NAME_SHOULD_FAIL(NAME) \
    TEST_NAME_SHOULD_FAIL_(TEST_NAME_PREFIX, NAME, TEST_NAME_SHOULD_FAIL_SUFFIX)

#define TEST(NAME) void TEST_NAME(NAME)(void)
#define TEST_SHOULD_FAIL(NAME) void TEST_NAME_SHOULD_FAIL(NAME)(void)

#define TEST_FAILURE_STRING "TEST_ASSERT failed"

// arg that tells test binary to print all test names
#define TEST_ARG_PRINT_TESTS "--tests"

// arg that tells test binary to run test
#define TEST_ARG_RUN_TEST "--test"
enum { TEST_ARG_RUN_TEST_LEN = CHARS_IN_STRING_LITERAL(TEST_ARG_RUN_TEST) };

typedef void (*TestFuncPtr)(void);

struct Test {
    const char* name;
    TestFuncPtr func;
};
typedef struct Test Test;

#if TESTS_ENABLED
#define TEST_ASSERT(CONDITION)                                        \
    do {                                                              \
        if (!(CONDITION)) {                                           \
            printf("test assertion failed: %s:%u (%s): assert(%s)\n", \
                   __FILE__, __LINE__, __func__, #CONDITION);         \
            exit(1);                                                  \
        }                                                             \
    } while (0)
#else
#define TEST_ASSERT(CONDITION) ((void)(CONDITION))
#endif

void run_test_from_bootargs(DeviceTree* device_tree);
