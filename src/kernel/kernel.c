#include <stdio.h>

#include "asm.h"
#include "build_info.h"
#include "debug.h"
#include "device_tree.h"
#include "example_process.h"
#include "flags.h"
#include "paging.h"
#include "panic.h"
#include "process.h"
#include "sections.h"
#include "trap.h"
#include "util.h"

__attribute__((section(".text.boot"))) NAKED void boot(void) {
    ASM(
        // set stack pointer
        "mv sp, %[stack]\n"

        // move device tree pointer from a1 to a0
        "mv a0, a1\n"

        // zero out a1 for good measure
        "mv a1, x0\n"

        // jump to kernel function
        "j " STRINGIFY(kernel_main) "\n"

        // load stack address from define
        ::[stack] "r"(STACK_END));
}

void kernel_main(const DeviceTreeHeadersRaw* device_tree_headers) {
    enable_trap_vector();
    enable_kernel_traps();

    init_kernel_page_table();

    printf(
        ""
        "----------\n"
        "Hello kernel_main\n");

    printf("(compiled with %s)\n", COMPILER_STRING);
    printf("\n");

    DeviceTree device_tree = parse_device_tree(device_tree_headers);

    if (DUMP_DEVICE_TREE) {
        DeviceTree_dump_raw(device_tree_headers);
        DeviceTree_print(&device_tree);
    }

    const char* args_path[] = {DEVICE_TREE_ROOT_PATH, "chosen", NULL};
    DeviceTreeNode* args =
        DeviceTreeNode_find_child(device_tree.root, (char**)args_path, 0);

    DeviceTreeProperty* boot_args =
        DeviceTreeNode_find_property(args, "bootargs");
    if (boot_args != NULL) {
        printf("bootargs = \"%s\"\n", (char*)boot_args->value);
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

    PANIC("end of kernel_main");
}
