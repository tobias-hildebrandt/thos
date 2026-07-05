#include "syscall.h"

#include "asm.h"
#include "syscalls.h"
#include "util.h"

NAKED long syscall(UNUSED long arg0, UNUSED long arg1, UNUSED long arg2,
                   UNUSED long arg3, UNUSED long arg4, UNUSED long arg5,
                   UNUSED long arg6, UNUSED long arg7) {
    ASM("ecall\n"
        "ret\n"
        // clobbers
        :: : "a0",
        "a1", "a2", "a3", "a4", "a5", "a6", "a7", "memory");
}

void yield(void) {
    SYSCALL(SYSCALL_YIELD);
}
