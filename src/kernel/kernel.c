#include "kernel.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "asm.h"
#include "build_info.h"
#include "debug.h"
#include "device/board.h"
#include "device/device_tree.h"
#include "device/sifive_uart.h"
#include "example_process.h"
#include "flags.h"
#include "paging.h"
#include "process.h"
#include "sections.h"
#include "test.h"
#include "trap.h"
#include "util.h"

static void kernel_main(uintptr_t hart_id,
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

    // TODO: move into a board init function
    if (board.sifive_uart1) {
        sifive_uart_init();
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

SECTION(".text.boot") UNUSED NAKED void boot(void) {
    // clang-format off
    ASM(
        // set stack pointer
        ASM_LOAD "sp, %[stack]\n"

        // jump to kernel function
        "j %[func]\n"
        ::
        [stack] "i"(&STACK_END),
        [func] "i"(kernel_main));
    // clang-format on
}
