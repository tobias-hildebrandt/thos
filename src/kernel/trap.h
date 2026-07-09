#pragma once

#include <stdint.h>

#include "asm.h"

struct TrapFrame {
    // return address, global pointer, thread pointer
    uintptr_t ra, gp, tp;
    // temporaries
    uintptr_t t0, t1, t2, t3, t4, t5, t6;
    // arguments
    uintptr_t a0, a1, a2, a3, a4, a5, a6, a7;
    // saved
    uintptr_t s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;
    // stack pointer
    uintptr_t sp;
    // program counter, stored from sepc
    uintptr_t pc;
    // used to transfer satp from switch_process to restore_after_trap
    uintptr_t satp;
} __attribute__((packed));
typedef struct TrapFrame TrapFrame;

void enable_trap_vector(void);
void enable_kernel_traps(void);
void restore_after_trap(TrapFrame* context);
void TrapFrame_print(TrapFrame* frame);

// 12.1.1.3 Supervisor Interrupt (sip and sie) Registers
// write SSIP to 0x2 for supervisor software interrupt
// clobbers a register!
#define KERNEL_SOFTWARE_INTERRUPT() ASM("csrw sip, %[val]\n" ::[val] "r"(0x2))
