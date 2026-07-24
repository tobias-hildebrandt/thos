#include "process/exits.h"

#include "asm.h"
#include "process/lifecycle.h"
#include "process/switch.h"
#include "sections.h"
#include "syscalls.h"
#include "trap/registers.h"
#include "util.h"

// user exit function
// located in user special function
// automatically set as return address when user process first switched on
NORETURN IN_USER_SPECIAL NAKED void ProcessExit_user(void) {
    ASM(
        // just do SYSCALL_EXIT and let kernel handle it
        "li a0, %0\n"
        "ecall\n"
        // fall through to fault if kernel doesn't clean
        "unimp\n" ::"i"(SYSCALL_EXIT));
}

// kernel process exit
// automatically set as return address when kernel process first switched on
NORETURN void ProcessExit_kernel(void) {
    disable_traps_now();

    // wipe current process (except kernel stack)
    Process_destroy_current();

    // switch into a new process
    jump_into_processes();
}
