#include "process/switch.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "asm.h"
#include "csr.h"
#include "flags.h"
#include "hart.h"
#include "io.h"
#include "lock.h"
#include "process/exits.h"
#include "process/process.h"
#include "timer.h"
#include "trap/frame.h"
#include "trap/registers.h"
#include "trap/routines.h"
#include "util.h"
#include "virtual_memory/satp.h"

Process processes[PROCESSES_MAXIMUM];
SpinLock processes_lock;

static Pid Pid_next(Pid id) {
    return (id + 1) % PROCESSES_MAXIMUM;
}

// Precondition: process list locked
static Process* Process_find_next(Pid start_id) {
    Pid id = start_id;

    Process* process;
    for (int num_checked = 0; num_checked < PROCESSES_MAXIMUM; num_checked++) {
        process = &processes[id];

        if (process->state == PROCESS_RUNNABLE ||
            process->state == PROCESS_READY) {
            // found a runnable process
            return process;
        }

        id = Pid_next(id);
    }
    return NULL;
}

// switches the hart scratch's current process to a new one.
// backs up old trapframe if hart's current proces is not NULL.
// TODO: scheduling algorithm
// TODO: refactor steps (backup, find new, restore) to functions
// Precondition: traps disabled
// Postcondition: hart_scratch->current_process != NULL
void kernel_switch(void) {
    HartScratch* hart_scratch = my_hart_scratch();
    TrapFrame* scratch_trap_frame = &hart_scratch->frame;
    uintptr_t hart_id = my_hart_id();

    uint8_t start_search_id = 0;

    // back up previous (current) process
    // can only happen if we started in trap_vector and didn't destroy process
    if (hart_scratch->current_process != NULL) {
        TrapFrame* process_frame = &(hart_scratch->current_process->frame);
        // copy frame into old context
        memcpy(process_frame, scratch_trap_frame, sizeof(TrapFrame));

        // if user process, we have to add 4 to PC
        if (!Process_is_kernel_process(hart_scratch->current_process)) {
            hart_scratch->current_process->frame.pc += 4;
        }
        PRINTF_IF(DEBUG_SWITCH,
                  "kernel_switch: hart 0x%x switch off pid %2d "
                  "(%-6s process), pc %p\n",
                  hart_id, hart_scratch->current_process->id,
                  Process_is_kernel_process(hart_scratch->current_process)
                      ? "kernel"
                      : "user",
                  process_frame->pc);
        if (DEBUG_SWITCH == 2) {
            printf("kernel_switch: hart 0x%x old context:\n", hart_id);
            TrapFrame_print(process_frame);
        }

        ASM("fence\n");

        start_search_id = Pid_next(hart_scratch->current_process->id);

        // other harts can now choose this process
        hart_scratch->current_process->state = PROCESS_RUNNABLE;

        hart_scratch->current_process = NULL;
    }

    assert(hart_scratch->current_process == NULL);

    bool process_first_run;

    // TODO: while no processes ready, suspend/shutdown hart
    // TODO: wake sleeping harts if mulitple processes ready
    while (1) {
        SpinLock_acquire(&processes_lock);

        Process* next_process = Process_find_next(start_search_id);

        if (next_process == NULL) {
            SpinLock_release(&processes_lock);
            continue;
            // PANIC("next_process NULL");
        } else {
            hart_scratch->current_process = next_process;

            process_first_run =
                (hart_scratch->current_process->state == PROCESS_READY);

            // process now running, other harts will not try to choose this
            hart_scratch->current_process->state = PROCESS_RUNNING;

            SpinLock_release(&processes_lock);
            break;
        }
    }

    // PC to sret into
    uintptr_t new_pc;

    if (process_first_run) {
        // first time this process is going to run

        // set sepc to return address (which is actually the entry point)
        new_pc = hart_scratch->current_process->frame.ra;

        // set up new return address
        if (Process_is_kernel_process(hart_scratch->current_process)) {
            hart_scratch->current_process->frame.ra =
                (uintptr_t)ProcessExit_kernel;
        } else {
            hart_scratch->current_process->frame.ra =
                (uintptr_t)ProcessExit_user;
        }
    } else {
        // go back to stored frame's PC
        new_pc = hart_scratch->current_process->frame.pc;
    }

    if (DEBUG_SWITCH) {
        printf(
            "kernel_switch: hart 0x%x switch on  pid %2d "
            "(%-6s process), pc %p\n",
            hart_id, hart_scratch->current_process->id,
            Process_is_kernel_process(hart_scratch->current_process) ? "kernel"
                                                                     : "user",
            new_pc);
    }

    csr_write_sepc(new_pc);

    // make sure kernel traps are active post-sret
    prepare_sstatus_for_return(
        Process_is_kernel_process(hart_scratch->current_process));

    // enable necessary supervisor interrupts (will only actually be active
    // after return)
    enable_interrupts();

    SatpRegister new_satp =
        SatpRegister_from_PageTable(hart_scratch->current_process->page_table);

    if (DEBUG_SWITCH == 2) {
        printf("kernel_switch: hart 0x%x new page table %p, new satp: %p\n",
               hart_id, hart_scratch->current_process->page_table,
               new_satp.value);
    }
    if (DEBUG_SWITCH == 2) {
        printf("kernel_switch: hart 0x%x new context:\n", hart_id);
        TrapFrame_print(&hart_scratch->current_process->frame);
    }

    // TODO: just set it in allocate_process?
    hart_scratch->current_process->frame.satp = new_satp.value;

    // copy process's trapframe into hart scratch's frame
    memcpy(&hart_scratch->frame, &hart_scratch->current_process->frame,
           sizeof(TrapFrame));

    // set interrupt timer
    set_timer(TIMER_INTERRUPT_DELAY);
}

// Entry point into processes from kernel main and kernel_exit
// Precondition: traps disabled
void NAKED NORETURN jump_into_processes(void) {
    // reset (clobber) stack to top of work stack
    ASM("csrr sp, sscratch\n"
        "li a0, %[offset]\n"
        "add sp, sp, a0\n"
        //
        ::[offset] "i"(offsetof(HartScratch, work_stack) + WORK_STACK_SIZE));

    // switch hart process to a new one (call with NULL argument)
    ASM("li a0, 0\n"
        "jal %0\n" ::"i"(kernel_switch));

    // restore process
    ASM("jal %0\n" ::"i"(restore_after_trap));
}
