#include "syscall.h"

#include <stdio.h>

#include "flags.h"  // IWYU pragma: keep
#include "hart.h"
#include "io.h"
#include "process/lifecycle.h"
#include "process/process.h"
#include "syscalls.h"
#include "trap/trap.h"

void handle_syscall(TrapFrame* frame) {
    switch (frame->a0) {
        case SYSCALL_YIELD: {
            PRINTF_IF(DEBUG_SYSCALL, "handle_syscall: yield\n");
            break;
        }
        case SYSCALL_PUTCHAR: {
            PRINTF_IF(DEBUG_SYSCALL, "handle_syscall: putchar\n");
            int ch = (int)frame->a1;
            putchar_buffer(ch, &my_hart_current_process()->output_buffer);
            frame->a0 = ch;
            break;
        }
        case SYSCALL_EXIT: {
            PRINTF_IF(DEBUG_SYSCALL, "handle_syscall: exit\n");
            // TODO: handle exit code in a1
            Process_destroy_current();
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
