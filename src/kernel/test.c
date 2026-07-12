#include "test.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>  // IWYU pragma: keep
#include <stdlib.h>
#include <string.h>  // IWYU pragma: keep

#include "device_tree.h"
#include "flags.h"  // IWYU pragma: keep
#include "io.h"
#include "panic.h"

const Test* get_test(char* name, const Test* tests, const size_t num_tests) {
    PRINTF_IF(DEBUG_TESTS, "looking for %s\n", name);
    for (size_t i = 0; i < num_tests; i++) {
        const Test* test = tests + i;
        PRINTF_IF(DEBUG_TESTS, "test %d = %s\n", i, test->name);
        if (0 == strcmp(name, test->name)) {
            return test;
        }
    }
    PANIC("no such test: %s", name);
    return NULL;
}

void run_test_from_bootargs(DeviceTree* device_tree) {
    if (!TESTS_ENABLED) {
        PANIC("run_test_from_bootargs but TEST_ENABLED is false");
        (void)device_tree;
    }

    const char* args_path[] = {DEVICE_TREE_ROOT_PATH, "chosen", NULL};
    DeviceTreeNode* chosen =
        DeviceTreeNode_find_child(device_tree->root, (char**)args_path, 0);

    DeviceTreeProperty* boot_args =
        DeviceTreeNode_find_property(chosen, "bootargs");
    if (boot_args == NULL) {
        PANIC("unable to find boot args");
    }

#if TESTS_ENABLED
    // provided by generated test_info.c
    extern const size_t num_tests;
    extern const Test* tests;

    char* boot_args_str = (char*)boot_args->value;

    // handle --tests
    if (0 == strcmp(boot_args_str, TEST_ARG_PRINT_TESTS)) {
        for (size_t i = 0; i < num_tests; i++) {
            const Test* test = tests + i;
            printf("%s\n", test->name);
        }
        exit(0);
    }

    // handle --testTEST
    if (0 == strncmp(boot_args_str, TEST_ARG_RUN_TEST, 6)) {
        // skip '--test' from boot_args
        const char* test_name = boot_args_str + TEST_ARG_RUN_TEST_LEN;
        const Test* test = get_test((char*)test_name, tests, num_tests);

        PRINTF_IF(DEBUG_TESTS, "running test %s\n", test->name);

        // run the test function
        (test->func)();

        exit(0);
    }

    PANIC("test binary run with argument that wasn't %s or %s",
          TEST_ARG_PRINT_TESTS, TEST_ARG_RUN_TEST);
#else
    PANIC("run_test_from_bootargs run when TEST_ENABLED undefined");
#endif
}

TEST(example_sanity_test) {
    printf("sanity check\n");
    TEST_ASSERT(1 == 1);
}

TEST_SHOULD_FAIL(expect_fail) {
    printf("should fail\n");
    TEST_ASSERT(true == false);
}
