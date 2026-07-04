#include "syscall.h"

#include <stdio.h>

#include "flags.h"
#include "io.h"
#include "syscalls.h"
#include "trap.h"

void handle_syscall(TrapFrame* frame) {
    switch (frame->a0) {
        case SYSCALL_YIELD: {
            PRINTF_IF(DEBUG_SYSCALL, "handle_syscall: yield\n");
            break;
        }
        case SYSCALL_PUTCHAR: {
            PRINTF_IF(DEBUG_SYSCALL, "handle_syscall: yield\n");
            int ch = frame->a1;
            int result = putchar(ch);
            frame->a0 = result;
            break;
        }
        default: {
            printf("handle_syscall: ignoring unknown syscall #%p\n", frame->a0);
            if (DEBUG_SYSCALL) {
                print_TrapFrame(frame);
            }
        }
    }
}
