#include "trap.h"

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
#include "paging.h"  // IWYU pragma: keep
#include "panic.h"
#include "process.h"
#include "sections.h"
#include "syscall.h"
#include "util.h"

#define PRINT_MEMBER(PTR, MEM) printf("%3s:\t%p\n", #MEM, PTR->MEM)

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

const char* decode_scause(uint64_t scause) {
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

// SPP privilege level before interrupt (1 is supervisor, 0 is user)
#define SSTATUS_SPP 8

void handle_trap(TrapFrame* frame) {
    uintptr_t scause = csr_read_scause();
    uintptr_t stval = csr_read_stval();
    uintptr_t sepc = csr_read_sepc();
    uintptr_t sstatus = csr_read_sstatus();

    bool was_in_kernel_mode = BIT_GET(sstatus, SSTATUS_SPP);
    bool software_interrupt = (scause == SUPERVISOR_SOFTWARE_INTERRUPT);
    bool ecall = (scause == ECALL_U_MODE);
    bool timer_interrupt = (scause == SUPERVISOR_TIMER_INTERRUPT);
    bool external_interrupt = (scause == SUPERVISOR_EXTERNAL_INTERRUPT);
    bool before_processes = current_process_stack_top == NULL;

    bool fatal =
        ((was_in_kernel_mode && !(software_interrupt || timer_interrupt)) ||
         (!was_in_kernel_mode && !(ecall))) &&
        !external_interrupt;

    int pid = current_process == NULL ? -1 : my_pid();

    // TODO: debug flag for external interrupts
    if ((was_in_kernel_mode && fatal) ||
        (((software_interrupt || ecall) && DEBUG_SOFTWARE_INTERRUPTS) ||
         (timer_interrupt && DEBUG_TIMER_INTERRUPTS) ||
         (external_interrupt && DEBUG_EXTERNAL_INTERRUPTS))) {
        printf(
            "\n***\ntrap: %s\n"
            "scause:  %p, stval: %p, sepc: %p\n"
            "sstatus: %p\n"
            "pid: %d, before_processes: %s\n"
            "(was in %s mode)\n",
            decode_scause(scause), scause, stval, sepc, sstatus, pid,
            before_processes ? "yes" : "no",
            was_in_kernel_mode ? "kernel" : "user");

        if (((software_interrupt || ecall) && DEBUG_SOFTWARE_INTERRUPTS == 2) ||
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
                PANIC("claimed unknown external interrupt: %d\n", interrupt);
            }

            sifive_plic_complete(1, interrupt);
        }

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

        clean_process();
        kernel_switch(NULL);
    } else {
        // handle fatal kernel trap
        printf("fatal trap\n");
        TrapFrame_print(frame);
        printf("***\n");
        PANIC("fatal trap");
    };
}

#define TRAP_STACK_OK "trap_vector_stack_ok"

// 12.1.2. Supervisor Trap Vector Base Address (stvec) Register
// "the address must be 4-byte aligned"
// TODO: __attribute__(interrupt)?
// https://gcc.gnu.org/onlinedocs/gcc/RISC-V-Attributes.html#index-interrupt_002c-RISC-V
IN_GLOBAL_SPECIAL NAKED __attribute__((aligned(4))) void trap_vector(void) {
    ASM(
        // need to swap to kernel page table NOW, before touching stack

        // store t0 in scratch to free it for page table
        "csrw sscratch, t0\n"

        // load kernel page satp into t0
        // located in global special page
        // which is mapped for both kernel and user processes
        "la t0, " STRINGIFY(kernel_page_satp) "\n"
        ASM_LOAD "t0, (t0)\n"

        // swap page table to t0
        "sfence.vma\n"
        "csrw satp, t0\n"
        "sfence.vma\n"

        // restore t0 from scratch
        "csrr t0, sscratch\n"

        // store process's stack pointer in scratch
        "csrw sscratch, sp\n"

        // change stack pointer to process's kernel stack
        // sp = (uintptr_t) current_process_stack_top;
        ASM_LOAD "sp, " STRINGIFY(current_process_stack_top) "\n"

        // if sp is not zero (trap before processes begin), jump ahead
        "bnez sp, "TRAP_STACK_OK"\n"
        // reload from scratch
        "csrr sp, sscratch\n"

        ASM_SET_LABEL(TRAP_STACK_OK)
        // add space on stack
        "addi sp, sp, -%[frame_size]\n"

        ::[frame_size]"i"(
            // make sure we keep stack aligned to 16
            align_up(sizeof(TrapFrame), 16)
        ));

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
        // store in stack's trapframe
        ASM_STORE "t0, %[offset](sp)\n"
        //
        ::[offset] "i"(offsetof(TrapFrame, sp)));

    // store sepc in stack's trapframe
    ASM("csrr t0, sepc\n"
        //
        ASM_STORE "t0, %[offset](sp)\n"
        //
        ::[offset] "i"(offsetof(TrapFrame, pc)));

    ASM(
        // point a0 (first function argument) to TrapFrame on stack
        "mv a0, sp\n"
        // call handle_trap
        "la t0, " STRINGIFY(handle_trap) "\n"
        "jr t0 \n");
}

// restores current_process context
// sepc should be set before jumping here!
// stack should be clean!
IN_GLOBAL_SPECIAL NAKED void restore_after_trap(UNUSED TrapFrame* context) {
    // start still in kernel page table

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

    ASM(
        // store real t0 in scratch
        "csrw sscratch, t0\n"
        // load satp into t0
        ASM_LOAD "t0, %[offset](a0)\n"
        //
        ::[offset] "i"(offsetof(TrapFrame, satp)));

    REGISTER_MEM(ASM_LOAD, a0, a0, TrapFrame);

    // restore given page table
    ASM("sfence.vma\n"
        "csrw satp, t0\n"
        "sfence.vma\n");

    // restore real t0
    ASM("csrr t0, sscratch\n");

    // return to whatever is in sepc
    ASM("sret\n");
}

// set up exception handler
void enable_trap_vector(void) {
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
    uintptr_t sstatus = csr_read_sstatus();
    BIT_UNSET(sstatus, SSTATUS_TRAPS_NOW);
    csr_write_sstatus(sstatus);
}

// enable SIE's supervisor software, timer, and external interrupt enable bits
void enable_interrupts(void) {
    uintptr_t sie = csr_read_sie();
    sie = BIT_TO_INT(SIE_SIP_SOFTWARE_INTERRUPT) |
          BIT_TO_INT(SIE_SIP_TIMER_INTERRUPT) |
          BIT_TO_INT(SIE_SIP_EXTERNAL_INTERRUPT);
    csr_write_sie(sie);
}
