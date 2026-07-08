#include <stdlib.h>

#include "syscall.h"
#include "syscalls.h"

// implement stdlib.h's exit
void exit(int exit_code) {
    SYSCALL(SYSCALL_EXIT, exit_code);
}
