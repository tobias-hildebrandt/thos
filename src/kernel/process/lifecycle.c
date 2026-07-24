#include "process/lifecycle.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "buffer.h"
#include "flags.h"
#include "hart.h"
#include "io.h"
#include "panic.h"
#include "process/process.h"
#include "process/switch.h"
#include "virtual_memory/page.h"
#include "virtual_memory/page_table.h"

Process* Process_create(ProcessArguments args) {
    Process* process = NULL;
    int index = 0;
    for (; index < PROCESSES_MAXIMUM; index++) {
        if (processes[index].state == PROCESS_UNUSED) {
            process = &processes[index];
            break;
        }
    }

    if (process == NULL) {
        PANIC("no more available processes");
    }

    // make sure we zero out everything including kernel stack
    memset(process, 0, sizeof(Process));

    // set ID
    process->id = index;

    // set up output buffer
    process->output_buffer = Buffer_wrap(process->out_buffer_array, BUFFER_LEN);

    // create page table and map memory
    if (args.is_user_program) {
        // TODO: copy into owned version
        // map process memory
        PageTable page_table = PageTable_user_init(
            USER_PROGRAM_BASE, args.entry_address, args.user_program_end);

        process->page_table = page_table;

        // when switched into, "returns" to the virtual address base
        // of all user programs
        process->frame.ra = USER_PROGRAM_BASE;

        // set stack pointer to user_program_end
        // TODO: maybe just leave it up to the user program?
        process->frame.sp = args.user_program_end;
    } else {
        // uses normal kernel page table
        process->page_table = kernel_page_table;

        // TODO: more stack space?
        // needs room for stack
        uintptr_t stack_page = (uintptr_t)Page_alloc();
        // points at top(!) of page
        process->frame.sp = stack_page + PAGE_SIZE;

        // when switched into, "returns" to the entry address like normal
        process->frame.ra = args.entry_address;
    }

    process->state = PROCESS_READY;
    // TODO: s0 might be the frame pointer, maybe do something with it?

    if (DEBUG_ALLOCATE_PROCESS) {
        printf("allocated ");
        Process_print(process);
    }

    return process;
}

// wipes the current process
// TODO: de-alloc user page table once alloc/de-alloc is implemented
// Precondition: traps disabled
void Process_destroy_current(void) {
    // TODO: lock process list

    HartScratch* hart_scratch = my_hart_scratch();

    PRINTF_IF(DEBUG_CLEAN_PROCESS, "clean_process: pid %d\n", my_pid());

    Process* old_process = hart_scratch->current_process;

    hart_scratch->current_process = NULL;

    if (old_process == NULL) {
        PANIC("clean_process called but no current_process");
    }

    // set process to unused
    old_process->id = -1;
    old_process->state = PROCESS_UNUSED;

    // TODO: de-alloc page table, memory, etc

    PRINTF_IF(DEBUG_CLEAN_PROCESS, "clean_process: wiped\n");
}
