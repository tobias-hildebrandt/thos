#include <stdio.h>

#include "syscall.h"
#include "syscalls.h"

// implements <stdio.h>'s do_putchar
int putchar(int ch) {
    return SYSCALL(SYSCALL_PUTCHAR, ch);
}
