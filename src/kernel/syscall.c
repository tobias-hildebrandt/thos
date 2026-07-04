#include "syscall.h"

#include <stdio.h>

#include "trap.h"

void handle_syscall(TrapFrame* frame) {
    if (frame->a0 == 1) {
        int ch = frame->a1;
        int result = putchar(ch);
        frame->a0 = result;
    } else {
        printf("handle_syscall: ignoring unknown syscall #%p\n", frame->a0);
        print_TrapFrame(frame);
    }
}
