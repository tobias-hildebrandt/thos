#include "boot/kernel.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "asm.h"
#include "build_info.h"
#include "csr.h"
#include "debug.h"
#include "device/board.h"
#include "device/device_tree.h"
#include "device/sifive_uart.h"
#include "example_process.h"
#include "flags.h"
#include "hart.h"
#include "lock.h"
#include "process/process.h"
#include "sbi.h"
#include "sections.h"
#include "test.h"
#include "trap/trap.h"
#include "util.h"
#include "virtual_memory/paging.h"

// TODO: clean up

// the first hart sets this to true, then wakes other harts
// other harts read this as true and don't touch it
static uint32_t first_claim = 0;

// initial printf lock, only used during kernel_main
static SpinLock stdout_lock;

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static void print_startup(const uintptr_t hart_id, uint32_t claim) {
    printf("Hello kernel_main\n");

    printf("sscratch:   %p\n", csr_read_sscratch());
    printf("a0 hartid:  %d\n", hart_id);
    printf("my_hart_id: %d\n", my_hart_id());
    printf("claim:      0x%x (%s)\n", claim,
           claim == 0 ? "first" : "not first");

    if (claim == 0) {
        printf("(compiled with %s)\n", COMPILER_STRING);
    }
    printf("\n");
}

static void kernel_main(const uintptr_t hart_id,
                        const DeviceTreeHeadersRaw* device_tree_headers) {
    uint32_t my_claim = atomic_or_memory_word(&first_claim, 0xffffffff);

    set_trap_vector();

    // not the first hart to run
    if (my_claim != 0) {
        SpinLock_acquire(&stdout_lock);
        print_startup(hart_id, my_claim);
        SpinLock_release(&stdout_lock);

        if (!EXAMPLE_PROCESSES_DISABLE) {
            reset_stack_begin_processes();
        }

        SpinLock_acquire(&stdout_lock);
        printf("end of kernel_main, hart id %d, hart claim 0x%x\n", hart_id,
               my_claim);
        SpinLock_release(&stdout_lock);

        sbi_hart_stop();

        while (1) {
        }
    }

    HartScratch_init_all();

    init_kernel_page_table();

    DeviceTree device_tree = DeviceTree_parse(device_tree_headers);

    if (TESTS_ENABLED) {
        run_test_from_bootargs(&device_tree);
    }

    if (DUMP_DEVICE_TREE) {
        SpinLock_acquire(&stdout_lock);
        {
            DeviceTree_dump_raw(device_tree_headers);
            DeviceTree_print(&device_tree);
        }
        SpinLock_release(&stdout_lock);
    }

    SpinLock_acquire(&stdout_lock);
    {
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
    }
    SpinLock_release(&stdout_lock);

    SpinLock_acquire(&stdout_lock);
    {
        print_startup(hart_id, my_claim);

        const unsigned long lowest_hart_to_wake = board.num_monitor_harts;
        const unsigned long highest_hart_to_wake = board.num_normal_harts;

        for (unsigned long hart_to_wake = lowest_hart_to_wake;
             hart_to_wake <= highest_hart_to_wake; hart_to_wake++) {
            if (hart_to_wake == hart_id) {
                printf("not starting own hart 0x%x\n", hart_to_wake);
                continue;
            }

            printf("starting hart id 0x%x\n", hart_to_wake);

            SbiReturn ret = sbi_hart_start(hart_to_wake, (unsigned long)boot,
                                           (unsigned long)device_tree_headers);
            if (0 != ret.error) {
                printf("hart_start return { error %d, value %d }\n", ret.error,
                       ret.value);
            }
        }
        printf("done starting harts\n\n");
    }
    SpinLock_release(&stdout_lock);

    if (!EXAMPLE_PROCESSES_DISABLE) {
        start_example_processes();

        reset_stack_begin_processes();
    }

    SpinLock_acquire(&stdout_lock);
    {
        printf("end of kernel_main, hart id %d, hart claim 0x%x\n", hart_id,
               my_claim);
    }
    SpinLock_release(&stdout_lock);

    // TODO: set return address in .boot and return?
    exit(0);
}

SECTION(".text.boot")
UNUSED NAKED void boot(UNUSED uintptr_t hart_id, UNUSED uintptr_t device_tree) {
    // CANNOT CLOBBER a0 OR a1 (or a2??)

    // load hart id into t0
    ASM("mv t0, a0\n");
    // load size of hart scratch space into t1
    ASM("li t1, %0\n" ::"i"(sizeof(HartScratch)));
    // calculate offset of this hart's scratch space from base into t1
    ASM("mul t1, t1, t0\n");
    // load base address of all hart scratch spaces into t0
    ASM("la t0, %0\n" ::"i"(hart_scratches));
    // add offset and base address into sp
    ASM("add sp, t0, t1\n");

    // set sscratch
    ASM("csrw sscratch, sp\n");

    // move sp to work stack
    ASM("addi sp, sp, %0\n" ::"i"(offsetof(HartScratch, work_stack)));

    // load stack size into t0
    ASM("li t0, %0\n" ::"i"(WORK_STACK_SIZE));
    // move sp to TOP of stack
    ASM("add sp, sp, t0\n");

    // jump to kernel function
    ASM("j %[func]\n" ::[func] "i"(kernel_main));
}
