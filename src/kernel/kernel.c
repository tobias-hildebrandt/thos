#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "asm.h"
#include "build_info.h"
#include "debug.h"
#include "device_tree.h"
#include "example_process.h"
#include "flags.h"
#include "paging.h"
#include "process.h"
#include "sections.h"
#include "test.h"
#include "trap.h"
#include "util.h"

__attribute__((section(".text.boot"))) NAKED void boot(void) {
    ASM(
        // set stack pointer
        "la sp, "STRINGIFY(STACK_END)"\n"
        "ld sp, (sp)\n"

        // jump to kernel function
        "j " STRINGIFY(kernel_main) "\n"

        : :: "sp");
}

void kernel_main(uintptr_t hart_id,
                 const DeviceTreeHeadersRaw* device_tree_headers) {
    enable_trap_vector();

    init_kernel_page_table();

    DeviceTree device_tree = DeviceTree_parse(device_tree_headers);

    if (TESTS_ENABLED) {
        run_test_from_bootargs(&device_tree);
    }

    printf(
        ""
        "----------\n"
        "Hello kernel_main\n");

    printf("hartid: %d\n", hart_id);

    printf("(compiled with %s)\n", COMPILER_STRING);
    printf("\n");

    if (DUMP_DEVICE_TREE) {
        DeviceTree_dump_raw(device_tree_headers);
        DeviceTree_print(&device_tree);
    }

    if (DEBUG_SECTIONS) {
        print_all_sections();
    }

    if (DEBUG_PRINTF) {
        debug_printf();
    }

    if (DEBUG_PAGE_ALLOC) {
        debug_page_alloc();
    }

    if (DEBUG_ATOI) {
        debug_atoi();
    }

    if (DEBUG_ALIGN) {
        debug_align();
    }

    if (!EXAMPLE_PROCESSES_DISABLE) {
        start_example_processes();

        begin_processes();
    }

    printf("end of kernel_main\n");

    // TODO: set return address in .boot?
    exit(0);
}
