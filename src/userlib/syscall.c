#include "syscall.h"

#include "asm.h"
#include "syscalls.h"

__attribute__((naked)) long syscall(long arg0, long arg1, long arg2, long arg3,
                                    long arg4, long arg5, long arg6,
                                    long arg7) {
    // environment call
    ASM("ecall\n"
        "ret\n"
        // clobbers
        :: : "a0",
        "a1", "a2", "a3", "a4", "a5", "a6", "a7", "memory");
}

void yield(void) {
    SYSCALL(SYSCALL_YIELD);
}
