#include "alloc.h"
#include "example_process.h"
#include "io.h"
#include "mem.h"  // IWYU pragma: keep, needed for memset
#include "panic.h"
#include "process.h"
#include "string.h"
#include "trap.h"

void kernel_main(void) {
    // read stack pointer into variable immediately
    uint64_t stack_pointer;
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

    for (int i = 0; i < 8; i++) {
        uint64_t page = alloc_page();
        printf("page %d alloc at 0x%x\n", i, page);
    }

    // trigger trap
    __asm__ __volatile__("unimp");

    printf("this is a line after unimp\n");

    // simple process that gets data from register s0
    Process* p1 = allocate_process((uint64_t)process_load_s0);
    p1->context.s0 = 111;

    // process that gets data from kernel stack
    Process* p2 = allocate_process((uint64_t)process_load_from_stack);
    // allocate on kernel stack
    p2->context.sp -= sizeof(SomeData);
    // put data on stack
    SomeData* p2_data = (SomeData*)(p2->kernel_stack + 8192 - sizeof(SomeData));
    p2_data->d = 222;
    p2_data->w = 10;
    memcpy(p2_data->b, "abcdefghijklmno", 16);
    p2_data->b[15] = 0;

    // process the eventually returns
    allocate_process((uint64_t)process_that_returns);

    // find a process and run it
    yield();

    // rest of body will not run
    PANIC("end of kernel_main");
}

extern char __STACK_START[];
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
