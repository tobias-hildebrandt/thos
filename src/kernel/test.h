#pragma once

#include <assert.h>  // IWYU pragma: keep
#include <stdio.h>   // IWYU pragma: keep
#include <stdlib.h>  // IWYU pragma: keep

#include "device_tree.h"
#include "util.h"

#define TEST_NAME(NAME) CONCAT_(_test_, NAME)
#define TEST(NAME) void TEST_NAME(NAME)(void)
#define TEST_FAILURE_STRING "TEST FAILED"
#define TEST_PASS_STRING "TEST PASSED"
#define TEST_NAME_PREFIX_LEN 7  // _test__

typedef void (*TestFuncPtr)(void);

struct Test {
    const char* name;
    TestFuncPtr func;
};
typedef struct Test Test;

#if TESTS_ENABLED
#define TEST_ASSERT(CONDITION)                       \
    do {                                             \
        if (CONDITION) {                             \
            printf("%s: %s\n", TEST_PASS_STRING,     \
                   __func__ + TEST_NAME_PREFIX_LEN); \
            exit(EXIT_SUCCESS);                      \
        } else {                                     \
            printf("%s: %s\n", TEST_FAILURE_STRING,  \
                   __func__ + TEST_NAME_PREFIX_LEN); \
            exit(EXIT_FAILURE);                      \
        }                                            \
    } while (0)
#else
#define TEST_ASSERT(CONDITION) ((void)(CONDITION))
#endif

void run_test_from_bootargs(DeviceTree* device_tree);
