#include "io.h"
#include "sbi.h"
#include "string.h"

extern char __STACK_START[];

void kernel_main(void) {
    int stack_pointer;
    // mv is psuedoinstruction for addi rd,rs,0
    __asm__ __volatile__("mv %0, sp"
                         // output
                         : "=r"(stack_pointer));

    const char* divider = "----------\n";
    put_c_str(divider);
    put_string(&STRING("Hello world!\n"));

    printf("sp at start of kernel_main: 0x%x\n\n", stack_pointer);

    printf("printf hex 0          = 0x%x\n", 0x0);
    printf("printf hex 0xffffffff = 0x%x\n", 0xffffffff);
    printf("printf hex 0x01234567 = 0x%x\n", 0x01234567);
    printf("printf hex 0x89abcdef = 0x%x\n", 0x89abcdef);

    const unsigned int LOOP_SPIN = 1024 * 1024;
    const unsigned int PER_LINE = 10;
    const unsigned int LINES = 10;

    for (unsigned int i = 0; i < LOOP_SPIN * LINES * PER_LINE; i++) {
        if (0 != i && i % (LOOP_SPIN * PER_LINE) == 0) {
            put_char('\n');
        }
        if (i % LOOP_SPIN == 0) {
            put_c_str(".");
        }
    }
    printf("\n");

    printf("shutting down!\n");
    sbi_shutdown();

    for (;;) {
        __asm__ __volatile__("wfi");
    }
}

__attribute__((section(".text.boot"))) __attribute__((naked)) void boot(void) {
    __asm__ __volatile__(
        // set stack pointer
        "mv sp, %0\n"
        // jump to kernel function
        "j kernel_main\n"
        // no outputs
        :
        // input
        : "r"(__STACK_START));
}
