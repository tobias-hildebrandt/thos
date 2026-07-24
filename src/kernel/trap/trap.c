#include "trap/trap.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "asm.h"
#include "bits.h"
#include "build_info.h"
#include "csr.h"
#include "device/board.h"
#include "device/sifive_plic.h"
#include "device/sifive_uart.h"
#include "flags.h"
#include "hart.h"
#include "lock.h"
#include "panic.h"
#include "process/process.h"
#include "sections.h"
#include "syscall.h"
#include "util.h"
#include "virtual_memory/paging.h"  // IWYU pragma: keep

#define PRINT_MEMBER(PTR, MEM) printf("%3s:\t%p\n", #MEM, (PTR)->MEM)

// 12.1.1.8. Supervisor Cause (scause) Register
#define SCAUSE(INTERRUPT, EXCEPTION) \
    ((INTERRUPT##UL << (POINTER_BITS - 1)) | (EXCEPTION##UL))
#define SCAUSE_CASE(INTERRUPT, EXCEPTION, STRING) \
    case SCAUSE(INTERRUPT, EXCEPTION):            \
        return STRING

#define SUPERVISOR_SOFTWARE_INTERRUPT SCAUSE(1, 1)
#define ECALL_U_MODE SCAUSE(0, 8)
#define SUPERVISOR_TIMER_INTERRUPT SCAUSE(1, 5)
#define SUPERVISOR_EXTERNAL_INTERRUPT SCAUSE(1, 9)

static const char* decode_scause(uint64_t scause) {
    switch (scause) {
            // clang-format off
        case SUPERVISOR_SOFTWARE_INTERRUPT: return "Supervisor software interrupt";
        case SUPERVISOR_TIMER_INTERRUPT: return "Supervisor timer interrupt";
        case SUPERVISOR_EXTERNAL_INTERRUPT: return "Supervisor external interrupt";
        SCAUSE_CASE(1, 13, "Counter-overflow interrupt");
        SCAUSE_CASE(0, 0, "Instruction address misaligned");
        SCAUSE_CASE(0, 1, "Instruction access fault");
        SCAUSE_CASE(0, 2, "Illegal instruction");
        SCAUSE_CASE(0, 3, "Breakpoint");
        SCAUSE_CASE(0, 4, "Load address misaligned");
        SCAUSE_CASE(0, 5, "Load access fault");
        SCAUSE_CASE(0, 6, "Store/AMO address misaligned");
        SCAUSE_CASE(0, 7, "Store/AMO access fault");
        case ECALL_U_MODE: return "Environment call from U-mode";
        SCAUSE_CASE(0, 9, "Environment call from S-mode");
        SCAUSE_CASE(0, 12, "Instruction page fault");
        SCAUSE_CASE(0, 13, "Load page fault");
        SCAUSE_CASE(0, 15, "Store/AMO page fault");
        SCAUSE_CASE(0, 18, "Software check");
        SCAUSE_CASE(0, 19, "Hardware error");
            // clang-format on
        default:
            return "(unknown)";
    }
}

void TrapFrame_print(TrapFrame* frame) {
    PRINT_MEMBER(frame, ra);
    PRINT_MEMBER(frame, sp);
    PRINT_MEMBER(frame, pc);

    PRINT_MEMBER(frame, gp);
    PRINT_MEMBER(frame, tp);

    PRINT_MEMBER(frame, t0);
    PRINT_MEMBER(frame, t1);
    PRINT_MEMBER(frame, t2);
    PRINT_MEMBER(frame, t3);
    PRINT_MEMBER(frame, t4);
    PRINT_MEMBER(frame, t5);
    PRINT_MEMBER(frame, t6);

    PRINT_MEMBER(frame, a0);
    PRINT_MEMBER(frame, a1);
    PRINT_MEMBER(frame, a2);
    PRINT_MEMBER(frame, a3);
    PRINT_MEMBER(frame, a4);
    PRINT_MEMBER(frame, a5);
    PRINT_MEMBER(frame, a6);
    PRINT_MEMBER(frame, a7);

    PRINT_MEMBER(frame, s0);
    PRINT_MEMBER(frame, s1);
    PRINT_MEMBER(frame, s2);
    PRINT_MEMBER(frame, s3);
    PRINT_MEMBER(frame, s4);
    PRINT_MEMBER(frame, s5);
    PRINT_MEMBER(frame, s6);
    PRINT_MEMBER(frame, s7);
    PRINT_MEMBER(frame, s8);
    PRINT_MEMBER(frame, s9);
    PRINT_MEMBER(frame, s10);
    PRINT_MEMBER(frame, s11);
}

static SpinLock trap_print_lock;

static NORETURN void handle_trap(TrapFrame* frame) {
    uintptr_t scause = csr_read_scause();
    uintptr_t stval = csr_read_stval();
    uintptr_t sepc = csr_read_sepc();
    uintptr_t sstatus = csr_read_sstatus();
    uintptr_t sscratch = csr_read_sscratch();
    uintptr_t hart_id = my_hart_id();

    HartScratch* hart_scratch = my_hart_scratch();

    bool was_in_kernel_mode = BIT_GET(sstatus, SSTATUS_PRIVILEGE);
    bool software_interrupt = (scause == SUPERVISOR_SOFTWARE_INTERRUPT);
    bool ecall = (scause == ECALL_U_MODE);
    bool timer_interrupt = (scause == SUPERVISOR_TIMER_INTERRUPT);
    bool external_interrupt = (scause == SUPERVISOR_EXTERNAL_INTERRUPT);
    bool not_in_process = hart_scratch->current_process == NULL;

    bool fatal =
        (((was_in_kernel_mode && !(software_interrupt || timer_interrupt)) ||
          (!was_in_kernel_mode && !ecall)) &&
         !external_interrupt) ||
        not_in_process;

    int pid = not_in_process ? -1 : my_pid();

    SpinLock_acquire(&trap_print_lock);
    // TODO: debug flag for external interrupts
    if ((was_in_kernel_mode && fatal) ||
        (((software_interrupt || ecall) && DEBUG_SOFTWARE_INTERRUPTS) ||
         (timer_interrupt && DEBUG_TIMER_INTERRUPTS) ||
         (external_interrupt && DEBUG_EXTERNAL_INTERRUPTS))) {
        printf(
            "\n***\ntrap: %s\n"
            "scause:  %p, stval: %p, sepc: %p\n"
            "sstatus: %p, sscratch: %p\n"
            "hart: %#x, pid: %d, not_in_process: %s\n"
            "(was in %s mode)\n",
            decode_scause(scause), scause, stval, sepc, sstatus, sscratch,
            hart_id, pid, not_in_process ? "yes" : "no",
            was_in_kernel_mode ? "kernel" : "user");

        if (fatal ||
            ((software_interrupt || ecall) && DEBUG_SOFTWARE_INTERRUPTS == 2) ||
            (timer_interrupt && DEBUG_TIMER_INTERRUPTS == 2) ||
            (external_interrupt && DEBUG_EXTERNAL_INTERRUPTS == 2)) {
            TrapFrame_print(frame);
        }
        printf("***\n");
    }
    // TODO: determine which other exceptions are recoverable?

    if (!fatal) {
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

        SpinLock_release(&trap_print_lock);
        // run switch
        kernel_switch(frame);
    } else if (!was_in_kernel_mode) {
        // handle fatal user traps
        if (DEBUG_USER_TRAPS) {
            printf(
                "\n***\nfatal user trap: %s\n"
                "scause:  %p, stval: %p, sepc: %p\n"
                "sstatus: %p\n"
                "pid: %d\n***\n",
                decode_scause(scause), scause, stval, sepc, sstatus, pid);
        } else {
            printf("killing pid %d: %s\n", pid, decode_scause(scause));
        }

        SpinLock_release(&trap_print_lock);
        clean_process();
        kernel_switch(NULL);
    } else {
        // handle fatal kernel trap
        SpinLock_release(&trap_print_lock);
        PANIC("fatal trap");
    }
}

#define TRAP_VECTOR_STACK_OK "trap_vector_stack_ok"

// 12.1.2. Supervisor Trap Vector Base Address (stvec) Register
// "the address must be 4-byte aligned"
// TODO: __attribute__(interrupt)?
// https://gcc.gnu.org/onlinedocs/gcc/RISC-V-Attributes.html#index-interrupt_002c-RISC-V
static NORETURN IN_GLOBAL_SPECIAL NAKED __attribute__((aligned(4))) void
trap_vector(void) {
    // hart scratch pointer currently in sscratch
    // hart scratch spaces are in global special, mapped for all processes

    // swap sp and scratch
    ASM("csrrw sp, sscratch, sp\n");

    // point SP to hart scratch's trap frame
    ASM("addi sp, sp, %0\n" ::"i"(offsetof(HartScratch, frame)));

    // store registers in stack's trap frame
    // starting registers
    REGISTER_MEM(ASM_STORE, sp, ra, TrapFrame);
    REGISTER_MEM(ASM_STORE, sp, gp, TrapFrame);
    REGISTER_MEM(ASM_STORE, sp, tp, TrapFrame);
    // t registers
    REGISTER_MEM(ASM_STORE, sp, t0, TrapFrame);
    REGISTER_MEM(ASM_STORE, sp, t1, TrapFrame);
    REGISTER_MEM(ASM_STORE, sp, t2, TrapFrame);
    REGISTER_MEM(ASM_STORE, sp, t3, TrapFrame);
    REGISTER_MEM(ASM_STORE, sp, t4, TrapFrame);
    REGISTER_MEM(ASM_STORE, sp, t5, TrapFrame);
    REGISTER_MEM(ASM_STORE, sp, t6, TrapFrame);
    // a registers
    REGISTER_MEM(ASM_STORE, sp, a0, TrapFrame);
    REGISTER_MEM(ASM_STORE, sp, a1, TrapFrame);
    REGISTER_MEM(ASM_STORE, sp, a2, TrapFrame);
    REGISTER_MEM(ASM_STORE, sp, a3, TrapFrame);
    REGISTER_MEM(ASM_STORE, sp, a4, TrapFrame);
    REGISTER_MEM(ASM_STORE, sp, a5, TrapFrame);
    REGISTER_MEM(ASM_STORE, sp, a6, TrapFrame);
    REGISTER_MEM(ASM_STORE, sp, a7, TrapFrame);
    // s registers
    REGISTER_MEM(ASM_STORE, sp, s0, TrapFrame);
    REGISTER_MEM(ASM_STORE, sp, s1, TrapFrame);
    REGISTER_MEM(ASM_STORE, sp, s2, TrapFrame);
    REGISTER_MEM(ASM_STORE, sp, s3, TrapFrame);
    REGISTER_MEM(ASM_STORE, sp, s3, TrapFrame);
    REGISTER_MEM(ASM_STORE, sp, s4, TrapFrame);
    REGISTER_MEM(ASM_STORE, sp, s5, TrapFrame);
    REGISTER_MEM(ASM_STORE, sp, s6, TrapFrame);
    REGISTER_MEM(ASM_STORE, sp, s7, TrapFrame);
    REGISTER_MEM(ASM_STORE, sp, s8, TrapFrame);
    REGISTER_MEM(ASM_STORE, sp, s9, TrapFrame);
    REGISTER_MEM(ASM_STORE, sp, s10, TrapFrame);
    REGISTER_MEM(ASM_STORE, sp, s11, TrapFrame);
    // pc, sp treated differently

    ASM(
        // read back old stack pointer from scratch
        "csrr t0, sscratch\n"
        // store in trapframe
        ASM_STORE "t0, %[offset](sp)\n"
        //
        ::[offset] "i"(offsetof(TrapFrame, sp)));

    ASM(
        // read pre-exception PC
        "csrr t0, sepc\n"
        // store in trapframe
        ASM_STORE "t0, %[offset](sp)\n"
        //
        ::[offset] "i"(offsetof(TrapFrame, pc)));

    // point SP to back to hart scratch space
    ASM("addi sp, sp, -%0\n" ::"i"(offsetof(HartScratch, frame)));

    // write hart scratch pointer back to scratch register
    ASM("csrw sscratch, sp\n");

    // need to swap to kernel page table before switching to kernel's stack

    // load kernel page satp into t0
    // located in global special page
    // which is mapped for both kernel and user processes
    ASM(ASM_LOAD "t0, %[satp_addr]\n" ::[satp_addr] "A"(kernel_page_satp));

    ASM("sfence.vma\n"
        "csrw satp, t0\n"
        "sfence.vma\n");

    // check if hart was in process or not
    ASM(ASM_LOAD "t0, %0(sp)\n" ::"i"(offsetof(HartScratch, current_process)));

    // if process was NULL, stay on current stack
    ASM("beqz t0, " TRAP_VECTOR_STACK_OK "\n");

    // if process was not NULL, switch to work stack
    ASM("li t0, %0\n"
        //
        ::"i"(offsetof(HartScratch, work_stack) + WORK_STACK_SIZE));
    ASM("add sp, sp, t0\n");

    // fall through to OK

    ASM(ASM_SET_LABEL(TRAP_VECTOR_STACK_OK));

    // point a0 (first function argument) to TrapFrame on stack
    ASM("csrr a0, sscratch\n"
        "addi a0, a0, %0\n" ::"i"(offsetof(HartScratch, frame)));

    // call handle_trap
    ASM("j %[trap_addr]\n" ::[trap_addr] "i"(handle_trap));
}

// restores context
// sepc should be set before jumping here!
NORETURN IN_GLOBAL_SPECIAL NAKED void restore_after_trap(
    UNUSED TrapFrame* frame) {
    // hart scratch pointer currently in sscratch
    // hart scratch spaces are in global special, mapped for all processes

    // start in kernel page table

    // load process's page table
    ASM(
        // load satp into t0
        ASM_LOAD "t0, %[offset](a0)\n"
        //
        ::[offset] "i"(offsetof(TrapFrame, satp)));

    // restore given page table
    ASM("sfence.vma\n"
        "csrw satp, t0\n"
        "sfence.vma\n");

    // load context
    // starting registers
    REGISTER_MEM(ASM_LOAD, a0, ra, TrapFrame);
    REGISTER_MEM(ASM_LOAD, a0, gp, TrapFrame);
    REGISTER_MEM(ASM_LOAD, a0, tp, TrapFrame);
    REGISTER_MEM(ASM_LOAD, a0, sp, TrapFrame);
    // t registers
    REGISTER_MEM(ASM_LOAD, a0, t0, TrapFrame);
    REGISTER_MEM(ASM_LOAD, a0, t1, TrapFrame);
    REGISTER_MEM(ASM_LOAD, a0, t2, TrapFrame);
    REGISTER_MEM(ASM_LOAD, a0, t3, TrapFrame);
    REGISTER_MEM(ASM_LOAD, a0, t4, TrapFrame);
    REGISTER_MEM(ASM_LOAD, a0, t5, TrapFrame);
    REGISTER_MEM(ASM_LOAD, a0, t6, TrapFrame);
    // a registers
    // restore a0 later!
    REGISTER_MEM(ASM_LOAD, a0, a1, TrapFrame);
    REGISTER_MEM(ASM_LOAD, a0, a2, TrapFrame);
    REGISTER_MEM(ASM_LOAD, a0, a3, TrapFrame);
    REGISTER_MEM(ASM_LOAD, a0, a4, TrapFrame);
    REGISTER_MEM(ASM_LOAD, a0, a5, TrapFrame);
    REGISTER_MEM(ASM_LOAD, a0, a6, TrapFrame);
    REGISTER_MEM(ASM_LOAD, a0, a7, TrapFrame);
    // s registers
    REGISTER_MEM(ASM_LOAD, a0, s0, TrapFrame);
    REGISTER_MEM(ASM_LOAD, a0, s1, TrapFrame);
    REGISTER_MEM(ASM_LOAD, a0, s2, TrapFrame);
    REGISTER_MEM(ASM_LOAD, a0, s3, TrapFrame);
    REGISTER_MEM(ASM_LOAD, a0, s3, TrapFrame);
    REGISTER_MEM(ASM_LOAD, a0, s4, TrapFrame);
    REGISTER_MEM(ASM_LOAD, a0, s5, TrapFrame);
    REGISTER_MEM(ASM_LOAD, a0, s6, TrapFrame);
    REGISTER_MEM(ASM_LOAD, a0, s7, TrapFrame);
    REGISTER_MEM(ASM_LOAD, a0, s8, TrapFrame);
    REGISTER_MEM(ASM_LOAD, a0, s9, TrapFrame);
    REGISTER_MEM(ASM_LOAD, a0, s10, TrapFrame);
    REGISTER_MEM(ASM_LOAD, a0, s11, TrapFrame);
    // sepc NOT restored here, it is handled beforehand

    // finally restore a0
    REGISTER_MEM(ASM_LOAD, a0, a0, TrapFrame);

    // return to whatever is in sepc
    ASM("sret\n");
}

// set up exception handler
void set_trap_vector(void) {
    // located in global special page
    // which is mapped for both kernel and user processes
    csr_write_stvec((uintptr_t)trap_vector);
}

// sets bits in sstatus to enable traps after return
void prepare_sstatus_for_return(bool return_to_kernel_mode) {
    uintptr_t sstatus = csr_read_sstatus();

    BIT_SET(sstatus, SSTATUS_TRAPS_AFTER_SRET);
    BIT_SET(sstatus, SSTATUS_SUM);
    BIT_BOOL(sstatus, SSTATUS_PRIVILEGE, return_to_kernel_mode);

    // printf("sstatus: %p\nsie: %p\n", sstatus, sie);

    // write
    csr_write_sstatus(sstatus);
}

// disable supervisor traps right now
void disable_traps_now(void) {
    csr_clear_mask_sstatus(BIT_TO_INT(SSTATUS_TRAPS_NOW));
}

// enable supervisor traps right now
void enable_traps_now(void) {
    csr_set_mask_sstatus(BIT_TO_INT(SSTATUS_TRAPS_NOW));
}

// enable SIE's supervisor software, timer, and external interrupt enable bits
void enable_interrupts(void) {
    uintptr_t sie = csr_read_sie();
    sie |= BIT_TO_INT(SIE_SIP_SOFTWARE_INTERRUPT) |
           BIT_TO_INT(SIE_SIP_TIMER_INTERRUPT) |
           BIT_TO_INT(SIE_SIP_EXTERNAL_INTERRUPT);
    csr_write_sie(sie);
}
