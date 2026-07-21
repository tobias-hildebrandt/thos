#include <stdlib.h>

#include "panic.h"
#include "syscall.h"
#include "syscalls.h"

// implement stdlib.h's exit
void __attribute__((noreturn)) exit(int exit_code) {
    SYSCALL(SYSCALL_EXIT, exit_code);
    PANIC("user program didn't exit after exit syscall");
}
