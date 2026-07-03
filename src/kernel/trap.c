#include "trap.h"

#include <stdint.h>
#include <stdio.h>

#include "asm.h"
#include "sbi.h"
#include "util.h"

struct TrapFrame {
    // return address, global pointer, thread pointer
    uint64_t ra, gp, tp;
    // temporaries
    uint64_t t0, t1, t2, t3, t4, t5, t6;
    // arguments
    uint64_t a0, a1, a2, a3, a4, a5, a6, a7;
    // saved
    uint64_t s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;
    // stack pointer
    uint64_t sp;
} __attribute__((packed));
typedef struct TrapFrame TrapFrame;

void handle_trap(TrapFrame* frame) {
    uint64_t scause;
    uint64_t stval;
    uint64_t sepc;

    (void)frame;

    __asm__ __volatile__("csrr %0, scause" : "=r"(scause));
    __asm__ __volatile__("csrr %0, stval" : "=r"(stval));
    __asm__ __volatile__("csrr %0, sepc" : "=r"(sepc));

    // TODO: actually handle traps
    printf(
        "kernel trap\n"
        "scause: 0x%016lx, stval: 0x%016lx, sepc: 0x%016lx\n",
        scause, stval, sepc);

    // TODO: determine which exceptions are fatal and which are recoverable

    if (scause == 0x2 && (stval & 0xffffffff) == 0xc0001073) {
        printf("skipping unimp!\n");

        // skip offending instruction
        // size is always 4 bytes, since we do NOT have compressed instructions
        __asm__ __volatile__("csrw sepc, %0" ::"r"(sepc + 4));
    } else {
        printf("fatal trap, shutting down\n");
        sbi_shutdown();
    };
}

// 12.1.2. Supervisor Trap Vector Base Address (stvec) Register
// "the address must be 4-byte aligned"
// TODO: __attribute__(interrupt)?
// https://gcc.gnu.org/onlinedocs/gcc/RISC-V-Attributes.html#index-interrupt_002c-RISC-V
__attribute__((naked)) __attribute__((aligned(4))) void trap_vector(void) {
    // TODO: need to swap to kernel page table NOW, before storing on stack

    ASM(
        // store current stack pointer in scratch
        "csrw sscratch, sp\n"
        // add space on stack
        "addi sp, sp, -%[frame_size]\n"
        // make sure we align to 16
        ::[frame_size] "i"(align_up(sizeof(TrapFrame), 16)));

    // store registers
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
        "call " STRINGIFY(handle_trap) "\n");

    // after call, load back registers
    // starting registers
    REGISTER_MEM(ld, sp, ra, TrapFrame);
    REGISTER_MEM(ld, sp, gp, TrapFrame);
    REGISTER_MEM(ld, sp, tp, TrapFrame);
    // t registers
    REGISTER_MEM(ld, sp, t0, TrapFrame);
    REGISTER_MEM(ld, sp, t1, TrapFrame);
    REGISTER_MEM(ld, sp, t2, TrapFrame);
    REGISTER_MEM(ld, sp, t3, TrapFrame);
    REGISTER_MEM(ld, sp, t4, TrapFrame);
    REGISTER_MEM(ld, sp, t5, TrapFrame);
    REGISTER_MEM(ld, sp, t6, TrapFrame);
    // a registers
    REGISTER_MEM(ld, sp, a0, TrapFrame);
    REGISTER_MEM(ld, sp, a1, TrapFrame);
    REGISTER_MEM(ld, sp, a2, TrapFrame);
    REGISTER_MEM(ld, sp, a3, TrapFrame);
    REGISTER_MEM(ld, sp, a4, TrapFrame);
    REGISTER_MEM(ld, sp, a5, TrapFrame);
    REGISTER_MEM(ld, sp, a6, TrapFrame);
    REGISTER_MEM(ld, sp, a7, TrapFrame);
    // s registers
    REGISTER_MEM(ld, sp, s0, TrapFrame);
    REGISTER_MEM(ld, sp, s1, TrapFrame);
    REGISTER_MEM(ld, sp, s2, TrapFrame);
    REGISTER_MEM(ld, sp, s3, TrapFrame);
    REGISTER_MEM(ld, sp, s3, TrapFrame);
    REGISTER_MEM(ld, sp, s4, TrapFrame);
    REGISTER_MEM(ld, sp, s5, TrapFrame);
    REGISTER_MEM(ld, sp, s6, TrapFrame);
    REGISTER_MEM(ld, sp, s7, TrapFrame);
    REGISTER_MEM(ld, sp, s8, TrapFrame);
    REGISTER_MEM(ld, sp, s9, TrapFrame);
    REGISTER_MEM(ld, sp, s10, TrapFrame);
    REGISTER_MEM(ld, sp, s11, TrapFrame);
    // restore old stack pointer
    REGISTER_MEM(ld, sp, sp, TrapFrame);
    // (technically could just subtract, would avoid a load op)
    /* ASM("addi sp, sp, %[frame_size]\n"
        ::[frame_size] "i"(align_up(sizeof(TrapFrame), 16)));
    */

    // return to where we were before
    ASM("sret\n");
}

// set up exception handler
void enable_trap_vector(void) {
    __asm__ __volatile__("csrw stvec, %0" ::"r"((uint64_t)trap_vector));
}
