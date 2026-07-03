#pragma once

#include "asm.h"

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

#include "process.h"

void enable_trap_vector(void);
void enable_kernel_traps(void);
void restore_after_trap(ProcessContext* context, SatpRegister satp);

// 12.1.1.3 Supervisor Interrupt (sip and sie) Registers
// write SSIP to 0x2 for supervisor software interrupt
// clobbers a register!
#define KERNEL_SOFTWARE_INTERRUPT() \
    ASM("csrw sip, %[val]\n"        \
                                    \
        ::[val] "r"(0x2))
