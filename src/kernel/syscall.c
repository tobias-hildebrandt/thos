#include "syscall.h"

#include <stdio.h>

#include "flags.h"  // IWYU pragma: keep
#include "io.h"
#include "process.h"
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
        case SYSCALL_EXIT: {
            PRINTF_IF(DEBUG_SYSCALL, "handle_syscall: exit\n");
            // TODO: handle exit code in a1
            clean_process();
            break;
        }
        default: {
            // TODO: return error in trapframe
            printf("handle_syscall: ignoring unknown syscall #%lu(%p)\n",
                   frame->a0, frame->a0);
            if (DEBUG_SYSCALL) {
                TrapFrame_print(frame);
            }
        }
    }
}
