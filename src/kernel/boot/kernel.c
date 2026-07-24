#include "boot/kernel.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "asm.h"
#include "boot/boot.h"
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
#include "process/switch.h"
#include "sbi.h"
#include "sections.h"
#include "test.h"
#include "trap/registers.h"
#include "util.h"
#include "virtual_memory/page_table.h"

// TODO: clean up

enum Claim {
    FIRST_CLAIM = 0,
    SECONDARY_CLAIM = INT32_MAX,
};
typedef enum Claim Claim;

static Claim claim = FIRST_CLAIM;

// initial printf lock, only used during kernel_main
static SpinLock stdout_lock;

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static void print_startup(const uintptr_t hart_id, const bool is_primary) {
    printf("Hello kernel_main\n");

    printf("sscratch:   %p\n", csr_read_sscratch());
    printf("a0 hartid:  %d\n", hart_id);
    printf("my_hart_id: %d\n", my_hart_id());
    printf("main type:  %s\n", is_primary == 0 ? "primary" : "secondary");

    if (is_primary) {
        printf("(compiled with %s)\n", COMPILER_STRING);
    }
    printf("\n");
}

static void NORETURN primary_main(
    const uintptr_t hart_id, const DeviceTreeHeadersRaw* device_tree_headers) {
    HartScratch_init_all();

    PageTable_kernel_init();

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
        print_startup(hart_id, true);

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

        jump_into_processes();
    }

    SpinLock_acquire(&stdout_lock);
    {
        printf("end of kernel_main, hart id %d\n", hart_id);
    }
    SpinLock_release(&stdout_lock);

    // TODO: set return address in .boot and return?
    exit(0);
}

static void NORETURN secondary_main(const uintptr_t hart_id) {
    SpinLock_acquire(&stdout_lock);
    print_startup(hart_id, false);
    SpinLock_release(&stdout_lock);

    if (!EXAMPLE_PROCESSES_DISABLE) {
        jump_into_processes();
    }

    SpinLock_acquire(&stdout_lock);
    printf("end of kernel_main, hart id %d\n", hart_id);
    SpinLock_release(&stdout_lock);

    sbi_hart_stop();

    while (1) {
    }
}

void NORETURN kernel_main(const uintptr_t hart_id,
                          const DeviceTreeHeadersRaw* device_tree_headers) {
    uint32_t my_claim = atomic_or_memory_word(&claim, SECONDARY_CLAIM);

    set_trap_vector();

    // not the first hart to run
    if (my_claim == 0) {
        primary_main(hart_id, device_tree_headers);
    } else {
        secondary_main(hart_id);
    }
}
