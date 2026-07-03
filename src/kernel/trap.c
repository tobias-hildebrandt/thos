#include "trap.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "asm.h"
#include "paging.h"
#include "panic.h"
#include "process.h"
#include "sections.h"
#include "util.h"

#define PRINT_MEM(PTR, MEM) printf(#MEM ":\t%p\n", PTR->MEM)

// 12.1.1.8. Supervisor Cause (scause) Register
#define SCAUSE(INTERRUPT, EXCEPTION) (INTERRUPT##L << 63 | EXCEPTION##L)
#define SCAUSE_CASE(INTERRUPT, EXCEPTION, STRING) \
    case SCAUSE(INTERRUPT, EXCEPTION):            \
        return STRING

const char* decode_scause(uint64_t scause) {
    switch (scause) {
        SCAUSE_CASE(1, 1, "Supervisor software interrupt");
        SCAUSE_CASE(1, 5, "Supervisor timer interrupt");
        SCAUSE_CASE(1, 9, "Supervisor external interrupt");
        SCAUSE_CASE(1, 13, "Counter-overflow interrupt");
        SCAUSE_CASE(0, 0, "Instruction address misaligned");
        SCAUSE_CASE(0, 1, "Instruction access fault");
        SCAUSE_CASE(0, 2, "Illegal instruction");
        SCAUSE_CASE(0, 3, "Breakpoint");
        SCAUSE_CASE(0, 4, "Load address misaligned");
        SCAUSE_CASE(0, 5, "Load access fault");
        SCAUSE_CASE(0, 6, "Store/AMO address misaligned");
        SCAUSE_CASE(0, 7, "Store/AMO access fault");
        SCAUSE_CASE(0, 8, "Environment call from U-mode");
        SCAUSE_CASE(0, 9, "Environment call from S-mode");
        SCAUSE_CASE(0, 12, "Instruction page fault");
        SCAUSE_CASE(0, 13, "Load page fault");
        SCAUSE_CASE(0, 15, "Store/AMO page fault");
        SCAUSE_CASE(0, 18, "Software check");
        SCAUSE_CASE(0, 19, "Hardware error");
        default:
            return "(unknown)";
    }
}

void print_TrapFrame(TrapFrame* frame) {
    PRINT_MEM(frame, ra);
    // PRINT_MEM(frame, gp);
    // PRINT_MEM(frame, tp);
    PRINT_MEM(frame, sp);

    // printf("\n");
    // PRINT_MEM(frame, t0);
    // PRINT_MEM(frame, t1);
    // PRINT_MEM(frame, t2);
    // PRINT_MEM(frame, t3);
    // PRINT_MEM(frame, t4);
    // PRINT_MEM(frame, t5);
    // PRINT_MEM(frame, t6);

    // PRINT_MEM(frame, a0);
    // PRINT_MEM(frame, a1);
    // PRINT_MEM(frame, a2);
    // PRINT_MEM(frame, a3);
    // PRINT_MEM(frame, a4);
    // PRINT_MEM(frame, a5);
    // PRINT_MEM(frame, a6);
    // PRINT_MEM(frame, a7);

    // PRINT_MEM(frame, s0);
    // PRINT_MEM(frame, s1);
    // PRINT_MEM(frame, s2);
    // PRINT_MEM(frame, s3);
    // PRINT_MEM(frame, s4);
    // PRINT_MEM(frame, s5);
    // PRINT_MEM(frame, s6);
    // PRINT_MEM(frame, s7);
    // PRINT_MEM(frame, s8);
    // PRINT_MEM(frame, s9);
    // PRINT_MEM(frame, s10);
    // PRINT_MEM(frame, s11);
}

void handle_trap(TrapFrame* frame) {
    uint64_t scause;
    uint64_t stval;
    uint64_t sepc;
    uint64_t sstatus;

    ASM("csrr %0, scause" : "=r"(scause));
    ASM("csrr %0, stval" : "=r"(stval));
    ASM("csrr %0, sepc" : "=r"(sepc));
    ASM("csrr %0, sstatus" : "=r"(sstatus));

    bool was_in_kernel_mode = (sstatus & (1 << 8)) >> 8;
    bool is_software_interrupt = (scause == 0x8000000000000001L);

    // TODO: actually handle traps
    if (!is_software_interrupt || DEBUG_SOFTWARE_INTERRUPTS) {
        printf(
            "\n***\nkernel trap: %s\n"
            "scause:  0x%016lx, stval: 0x%016lx, sepc: 0x%016lx\n"
            "sstatus: 0x%016lx\n"
            "(was in %s mode)\n",
            decode_scause(scause), scause, stval, sepc, sstatus,
            was_in_kernel_mode ? "kernel" : "user");
    }

    // TODO: determine which other exceptions are recoverable?

    if (is_software_interrupt) {
        // clear interrupt
        ASM("csrw sip, %[val]\n" ::[val] "r"(0x0));

        // run switch
        kernel_switch(frame);
    } else {
        printf("fatal trap\n");
        print_TrapFrame(frame);
        printf("***\n");
        PANIC("fatal trap");
    };
}

// 12.1.2. Supervisor Trap Vector Base Address (stvec) Register
// "the address must be 4-byte aligned"
// TODO: __attribute__(interrupt)?
// https://gcc.gnu.org/onlinedocs/gcc/RISC-V-Attributes.html#index-interrupt_002c-RISC-V
IN_USER_SPECIAL __attribute__((naked)) __attribute__((aligned(4))) void
trap_vector(void) {
    ASM(
        // need to swap to kernel page table NOW, before touching stack

        // store t0 in scratch to free it for page table
        "csrw sscratch, t0\n"

        // load kernel page satp into t0
        // located in user special page
        // which is mapped for both kernel and user processes
        "la t0, " STRINGIFY(kernel_page_satp) "\n"
        "ld t0, (t0)\n"

        // swap page table to t0
        "sfence.vma\n"
        "csrw satp, t0\n"
        "sfence.vma\n"

        // restore t0 from scratch
        "csrr t0, sscratch\n"

        // store process's stack pointer in scratch
        "csrw sscratch, sp\n"

        // TODO: need to change stack pointer for user programs?

        // add space on stack
        "addi sp, sp, -%[frame_size]\n"
        // make sure we align to 16
        ::[frame_size] "i"(align_up(sizeof(TrapFrame), 16)));

    // store registers in stack's trap frame
    // starting registers
    REGISTER_MEM(sd, sp, ra, TrapFrame);
    REGISTER_MEM(sd, sp, gp, TrapFrame);
    REGISTER_MEM(sd, sp, tp, TrapFrame);
    // t registers
    REGISTER_MEM(sd, sp, t0, TrapFrame);
    REGISTER_MEM(sd, sp, t1, TrapFrame);
    REGISTER_MEM(sd, sp, t2, TrapFrame);
    REGISTER_MEM(sd, sp, t3, TrapFrame);
    REGISTER_MEM(sd, sp, t4, TrapFrame);
    REGISTER_MEM(sd, sp, t5, TrapFrame);
    REGISTER_MEM(sd, sp, t6, TrapFrame);
    // a registers
    REGISTER_MEM(sd, sp, a0, TrapFrame);
    REGISTER_MEM(sd, sp, a1, TrapFrame);
    REGISTER_MEM(sd, sp, a2, TrapFrame);
    REGISTER_MEM(sd, sp, a3, TrapFrame);
    REGISTER_MEM(sd, sp, a4, TrapFrame);
    REGISTER_MEM(sd, sp, a5, TrapFrame);
    REGISTER_MEM(sd, sp, a6, TrapFrame);
    REGISTER_MEM(sd, sp, a7, TrapFrame);
    // s registers
    REGISTER_MEM(sd, sp, s0, TrapFrame);
    REGISTER_MEM(sd, sp, s1, TrapFrame);
    REGISTER_MEM(sd, sp, s2, TrapFrame);
    REGISTER_MEM(sd, sp, s3, TrapFrame);
    REGISTER_MEM(sd, sp, s3, TrapFrame);
    REGISTER_MEM(sd, sp, s4, TrapFrame);
    REGISTER_MEM(sd, sp, s5, TrapFrame);
    REGISTER_MEM(sd, sp, s6, TrapFrame);
    REGISTER_MEM(sd, sp, s7, TrapFrame);
    REGISTER_MEM(sd, sp, s8, TrapFrame);
    REGISTER_MEM(sd, sp, s9, TrapFrame);
    REGISTER_MEM(sd, sp, s10, TrapFrame);
    REGISTER_MEM(sd, sp, s11, TrapFrame);

    ASM(
        // read back old stack pointer from scratch
        "csrr t0, sscratch\n"
        // store in stack's trapframe
        "sd t0, %[offset](sp)\n"
        //
        ::[offset] "i"(offsetof(TrapFrame, sp)));

    ASM(
        // point a0 (first function argument) to TrapFrame on stack
        "mv a0, sp\n"
        // call handle_trap
        "j " STRINGIFY(handle_trap) "\n");
}

// restores current_process context
// sepc should be set before jumping here!
// stack should be clean!
IN_USER_SPECIAL __attribute__((naked)) void restore_after_trap(
    ProcessContext* context, SatpRegister satp) {
    // start still in kernel page table

    // load context
    REGISTER_MEM(ld, a0, ra, ProcessContext);
    REGISTER_MEM(ld, a0, sp, ProcessContext);
    REGISTER_MEM(ld, a0, s0, ProcessContext);
    REGISTER_MEM(ld, a0, s1, ProcessContext);
    REGISTER_MEM(ld, a0, s2, ProcessContext);
    REGISTER_MEM(ld, a0, s3, ProcessContext);
    REGISTER_MEM(ld, a0, s4, ProcessContext);
    REGISTER_MEM(ld, a0, s5, ProcessContext);
    REGISTER_MEM(ld, a0, s6, ProcessContext);
    REGISTER_MEM(ld, a0, s7, ProcessContext);
    REGISTER_MEM(ld, a0, s8, ProcessContext);
    REGISTER_MEM(ld, a0, s9, ProcessContext);
    REGISTER_MEM(ld, a0, s10, ProcessContext);
    REGISTER_MEM(ld, a0, s11, ProcessContext);

    // restore given page table
    ASM("sfence.vma\n"
        "csrw satp, a1\n"
        "sfence.vma\n");

    ASM("nop\n");

    // return to whatever is in sepc
    ASM("sret\n");
}

// set up exception handler
void enable_trap_vector(void) {
    // located in user special page
    // which is mapped for both kernel and user processes
    ASM("csrw stvec, %0" ::"r"((uint64_t)trap_vector));
}

// 12.1.1.3. Supervisor Interrupt (sip and sie) Registers
void enable_kernel_traps(void) {
    uint64_t sstatus;
    uint64_t sie;
    // read
    ASM("csrr %[sstatus], sstatus\n"
        "csrr %[sie], sie\n"
        //
        : [sstatus] "=r"(sstatus),
        [sie] "=r"(sie));

    // printf("sstatus: %p\nsie: %p\n", sstatus, sie);

    // set bits
    // SIE bit
    sstatus |= 0x2;
    // SSIE bit
    sie |= 0x2;

    // write
    ASM("csrw sstatus, %[sstatus]\n"
        "csrw sie, %[sie]\n"
        //
        ::[sstatus] "r"(sstatus),
        [sie] "r"(sie));
}
