#pragma once

#include <stdint.h>

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
    // TODO: just pass a separate argument instead of storing space here?
    // used to transfer satp from switch_process to restore_after_trap
    uintptr_t satp;
} __attribute__((packed));
typedef struct TrapFrame TrapFrame;

void TrapFrame_print(TrapFrame* frame);
