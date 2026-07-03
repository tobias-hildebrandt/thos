#include <stdio.h>

#include "asm.h"
#include "debug.h"
#include "example_process.h"
#include "flags.h"
#include "paging.h"
#include "panic.h"
#include "process.h"
#include "sections.h"
#include "trap.h"
#include "util.h"

__attribute__((section(".text.boot"))) __attribute__((naked)) void boot(void) {
    ASM(
        // set stack pointer
        "mv sp, %0\n"
        // jump to kernel function
        "j " STRINGIFY(kernel_main) "\n"
        // no outputs
        :
        // input
        // stack starts at top of stack section and grows down in address space
        : "r"(STACK_END));
}

void kernel_main(void) {
    enable_trap_vector();
    enable_kernel_traps();

    printf(
        ""
        "----------\n"
        "Hello kernel_main\n");

    init_kernel_page_table();

    print_all_sections();

    if (DEBUG_PRINTF) {
        debug_printf();
    }

    if (DEBUG_PAGE_ALLOC) {
        debug_page_alloc();
    }

    if (DEBUG_ATOI) {
        debug_atoi();
    }

    if (!EXAMPLE_PROCESSES_DISABLE) {
        start_example_processes();

        begin_processes();
    }

    PANIC("end of kernel_main");
}
