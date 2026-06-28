#include "io.h"
#include "panic.h"
#include "sbi.h"
#include "string.h"
#include "trap.h"

extern char __STACK_START[];

void kernel_main(void) {
    // read stack pointer into variable immediately
    int stack_pointer;
    __asm__ __volatile__("mv %0, sp" : "=r"(stack_pointer));

    // set up exception handler
    __asm__ __volatile__("csrw stvec, %0" ::"r"((uint64_t)trap_vector));

    const char* divider = "----------\n";
    put_c_str(divider);
    put_string(&STRING("Hello world!\n"));

    printf("%%sp = 0x%x\n\n", stack_pointer);

    printf("printf hex 0          = 0x%x\n", 0x0);
    printf("printf hex 0xffffffff = 0x%x\n", 0xffffffff);
    printf("printf hex 0x01234567 = 0x%x\n", 0x01234567);
    printf("printf hex 0x89abcdef = 0x%x\n", 0x89abcdef);

    for (int i = -10; i <= 10; i++) {
        printf("%d ", i);
    }
    printf("\n");

    for (int i = -100000; i <= 100000; i += 10000) {
        printf("0x%x=%d\n", i, i);
    }
    printf("zero        0x%x=%d\n", 0, 0);
    printf("max int     0x%x=%d\n", INT_MAX, INT_MAX);
    printf("all bits 1  0x%x=%d\n", 0xffffffff, 0xffffffff);
    printf("min int     0x%x=%d\n", INT_MIN, INT_MIN);

    const unsigned int LOOP_SPIN = 1024 /* * 1024 */;
    const unsigned int PER_LINE = 10;
    const unsigned int LINES = 10;

    printf("spinning now\n");

    for (unsigned int i = 0; i < LOOP_SPIN * LINES * PER_LINE; i++) {
        if (0 != i && i % (LOOP_SPIN * PER_LINE) == 0) {
            put_char('\n');
        }
        if (i % LOOP_SPIN == 0) {
            put_char('.');
        }
    }
    printf("\n");

    // trigger trap
    __asm__ __volatile__("unimp");

    printf("this is a line after unimp\n");

    PANIC("this is an expected panic");

    // rest of body will not run

    printf("should not be printed!\n");
    sbi_shutdown();
}

__attribute__((section(".text.boot"))) __attribute__((naked)) void boot(void) {
    __asm__ __volatile__(
        // set stack pointer
        "mv sp, %0\n"
        // jump to kernel function
        "j " STRINGIFY(kernel_main) "\n"
        // no outputs
        :
        // input
        : "r"(__STACK_START));
}
