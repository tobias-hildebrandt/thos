#pragma once

enum SyscallNumber {
    SYSCALL_EXIT = 0x0F,
    SYSCALL_YIELD = 0x10,
    SYSCALL_PUTCHAR = 0xAA,
};
