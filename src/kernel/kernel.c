#include <stdio.h>

#include "debug.h"
#include "example_process.h"
#include "flags.h"
#include "panic.h"
#include "process.h"
#include "sections.h"
#include "trap.h"
#include "util.h"

__attribute__((section(".text.boot"))) __attribute__((naked)) void boot(void) {
    __asm__ __volatile__(
        // set stack pointer
        "mv sp, %0\n"
        // jump to kernel function
        "j " STRINGIFY(kernel_main) "\n"
        // no outputs
        :
        // input
        : "r"(__STACK_START));
}

void kernel_main(void) {
    enable_trap_vector();

    printf(
        ""
        "----------\n"
        "Hello kernel_main\n");

    print_memory_size();

    if (DEBUG_PRINTF) {
        debug_printf();
    }

    if (DEBUG_TRAP) {
        debug_trap();
    }

    if (DEBUG_PAGE_ALLOC) {
        debug_page_alloc();
    }

    if (!EXAMPLE_PROCESSES_DISABLE) {
        start_example_processes();

        yield();
    }

    PANIC("end of kernel_main");
}
