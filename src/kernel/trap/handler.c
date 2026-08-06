#include "trap/handler.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "bits.h"
#include "csr.h"
#include "device/board.h"
#include "device/sifive_plic.h"
#include "device/sifive_uart.h"
#include "flags.h"
#include "hart.h"
#include "lock.h"
#include "panic.h"
#include "process/lifecycle.h"
#include "process/process.h"
#include "process/switch.h"
#include "syscall.h"
#include "trap/frame.h"
#include "trap/routines.h"
#include "trap/scause.h"
#include "util.h"

static SpinLock trap_print_lock;

NORETURN void handle_trap(void) {
    uintptr_t scause = csr_read_scause();
    uintptr_t stval = csr_read_stval();
    uintptr_t sepc = csr_read_sepc();
    uintptr_t sstatus = csr_read_sstatus();
    uintptr_t hart_id = my_hart_id();

    HartScratch* hart_scratch = my_hart_scratch();
    TrapFrame* frame = &hart_scratch->frame;

    bool was_in_kernel_mode = BIT_GET(sstatus, SSTATUS_PRIVILEGE);
    bool software_interrupt = (scause == SCAUSE_SUPERVISOR_SOFTWARE_INTERRUPT);
    bool ecall = (scause == SCAUSE_ECALL_U_MODE);
    bool timer_interrupt = (scause == SCAUSE_SUPERVISOR_TIMER_INTERRUPT);
    bool external_interrupt = (scause == SCAUSE_SUPERVISOR_EXTERNAL_INTERRUPT);
    bool was_in_process = hart_scratch->current_process != NULL;

    // TODO: clean up conditions

    bool fatal =
        (((was_in_kernel_mode && !(software_interrupt || timer_interrupt)) ||
          (!was_in_kernel_mode && !(ecall || timer_interrupt))) &&
         !external_interrupt) ||
        !was_in_process;

    int pid = was_in_process ? (int)my_pid() : -1;

    SpinLock_acquire(&trap_print_lock);
    if ((fatal && (was_in_kernel_mode || DEBUG_USER_TRAPS)) ||
        ((software_interrupt && DEBUG_SOFTWARE_INTERRUPTS) ||
         (ecall && DEBUG_SYSCALL) ||
         (timer_interrupt && DEBUG_TIMER_INTERRUPTS) ||
         (external_interrupt && DEBUG_EXTERNAL_INTERRUPTS))) {
        printf(
            "\n***\ntrap: %s\n"
            "scause:  %p, stval: %p, sepc: %p\n"
            "sstatus: %p, hart_scratch: %p\n"
            "hart: %#x, pid: %d, was_in_process: %s\n"
            "(was in %s mode)\n",
            decode_scause(scause), scause, stval, sepc, sstatus, hart_scratch,
            hart_id, pid, was_in_process ? "yes" : "no",
            was_in_kernel_mode ? "kernel" : "user");

        if ((fatal && (was_in_kernel_mode ||
                       (!was_in_kernel_mode && DEBUG_USER_TRAPS == 2))) ||
            ((software_interrupt && DEBUG_SOFTWARE_INTERRUPTS == 2) ||
             (ecall && DEBUG_SYSCALL == 2) ||
             (timer_interrupt && DEBUG_TIMER_INTERRUPTS == 2) ||
             (external_interrupt && DEBUG_EXTERNAL_INTERRUPTS == 2))) {
            TrapFrame_print(frame);
        }
        printf("***\n");
    }
    // TODO: determine which other exceptions are recoverable?

    if (!fatal) {
        // nonfatal trap

        // clear all interrupts
        // TODO: only clear given interrupt
        csr_write_sip(0x0);

        if (ecall) {
            handle_syscall(frame);
        }

        // TODO: move to own handler
        if (external_interrupt) {
            GET_BOARD_DEVICE(board.sifive_plic);
            GET_BOARD_DEVICE(board.sifive_uart1);

            uint32_t interrupt = sifive_plic_claim(1);
            if (interrupt == SIFIVE_UART1_INTERRUPT) {
                sifive_uart_drain();
            } else {
                SpinLock_release(&trap_print_lock);
                PANIC("claimed unknown external interrupt: %d\n", interrupt);
            }

            sifive_plic_complete(1, interrupt);
        }

        // run switch
        kernel_switch();
    } else {
        // fatal trap
        if (!was_in_kernel_mode) {
            // handle fatal user traps
            if (!DEBUG_USER_TRAPS) {
                printf("killing pid %d: %s\n", pid, decode_scause(scause));
            }

            Process_destroy_current();
            kernel_switch();
        } else {
            // handle fatal kernel trap
            SpinLock_release(&trap_print_lock);
            PANIC("fatal trap");
        }
    }

    SpinLock_release(&trap_print_lock);

    restore_after_trap();
}
