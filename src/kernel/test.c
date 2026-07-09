#include "test.h"

#include <stdbool.h>
#include <stdio.h>   // IWYU pragma: keep
#include <string.h>  // IWYU pragma: keep

#include "device_tree.h"
#include "flags.h"  // IWYU pragma: keep
#include "panic.h"

TestFuncPtr get_test(char* name, const Test* tests, const size_t num_tests) {
    for (size_t i = 0; i < num_tests; i++) {
        const Test* test = tests + i;
        if (0 == strcmp(name, test->name)) {
            return test->func;
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

    char* boot_args_str = (char*)boot_args->value;
    printf("bootargs = \"%s\"\n", boot_args_str);

#if TESTS_ENABLED
    // provided by generated test_info.c
    extern const size_t num_tests;
    extern const Test* tests;

    TestFuncPtr test = get_test(boot_args_str, tests, num_tests);
    // skip "_test__"
    printf("running test %s\n", boot_args_str + TEST_NAME_PREFIX_LEN);
    (test)();
#else
    PANIC("run_test_from_bootargs somehow got passed TEST_ENABLED check");
#endif

    exit(0);
}

TEST(example_sanity_test) {
    printf("sanity check\n");
    TEST_ASSERT(1 == 1);
}

TEST(expect_fail) {
    printf("should fail\n");
    TEST_ASSERT(true == false);
}
