#include "trap/routines.h"

#include <stdbool.h>
#include <stddef.h>

#include "asm.h"
#include "hart.h"
#include "sections.h"
#include "trap/frame.h"
#include "trap/handler.h"
#include "util.h"
#include "virtual_memory/satp.h"

#define TRAP_VECTOR_STACK_OK "trap_vector_stack_ok"

// 12.1.2. Supervisor Trap Vector Base Address (stvec) Register
// "the address must be 4-byte aligned"
// TODO: __attribute__(interrupt)?
// https://gcc.gnu.org/onlinedocs/gcc/RISC-V-Attributes.html#index-interrupt_002c-RISC-V
NORETURN IN_GLOBAL_SPECIAL NAKED __attribute__((aligned(4))) void trap_vector(
    void) {
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
