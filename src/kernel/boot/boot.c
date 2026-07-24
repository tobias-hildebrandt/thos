#include "boot/boot.h"

#include <stddef.h>
#include <stdint.h>

#include "asm.h"
#include "boot/kernel.h"
#include "hart.h"
#include "util.h"

SECTION(".text.boot")
UNUSED NAKED void boot(UNUSED uintptr_t hart_id, UNUSED uintptr_t device_tree) {
    // CANNOT CLOBBER a0 OR a1 (or a2??)

    // load hart id into t0
    ASM("mv t0, a0\n");
    // load size of hart scratch space into t1
    ASM("li t1, %0\n" ::"i"(sizeof(HartScratch)));
    // calculate offset of this hart's scratch space from base into t1
    ASM("mul t1, t1, t0\n");
    // load base address of all hart scratch spaces into t0
    ASM("la t0, %0\n" ::"i"(hart_scratches));
    // add offset and base address into sp
    ASM("add sp, t0, t1\n");

    // set sscratch
    ASM("csrw sscratch, sp\n");

    // move sp to work stack
    ASM("addi sp, sp, %0\n" ::"i"(offsetof(HartScratch, work_stack)));

    // load stack size into t0
    ASM("li t0, %0\n" ::"i"(WORK_STACK_SIZE));
    // move sp to TOP of stack
    ASM("add sp, sp, t0\n");

    // jump to kernel function
    ASM("j %[func]\n" ::[func] "i"(kernel_main));
}
