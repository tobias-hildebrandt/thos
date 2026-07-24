#include "trap/frame.h"

#include <stdio.h>

#define PRINT_MEMBER(PTR, MEM) printf("%3s:\t%p\n", #MEM, (PTR)->MEM)

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
