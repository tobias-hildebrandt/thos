#include "alloc.h"
#include "example_process.h"
#include "io.h"
#include "mem.h"  // IWYU pragma: keep, needed for memset
#include "paging.h"
#include "panic.h"
#include "process.h"
#include "string.h"
#include "trap.h"

extern char __KERNEL_BASE[];

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

    // print testing
    {
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
    }

    // page alloc testing
    {
        for (int i = 0; i < 8; i++) {
            uint64_t page = alloc_page();
            *(uint64_t*)page = i;
            printf("page alloc at 0x%x\n", page);
        }
    }

    // trap test
    {
        // trigger trap
        __asm__ __volatile__("unimp");

        printf("this is a line after unimp\n");
    }

    // enable virtual memory
    {
        const uint64_t memory_start = (uint64_t)__KERNEL_BASE;
        const uint64_t memory_end = (uint64_t)__PAGES_END;
        const uint64_t total_memory_space = memory_end - memory_start;
        const uint64_t total_pages = total_memory_space / PAGE_SIZE;
        printf("kernel memory space: %dB = %d pages\n", total_memory_space,
               total_pages);

        PageTable kernel_table = (PageTable)alloc_page();

        // TODO: mega/giga pages for different sections?

        // map entire kernel physical memory space
        for (uint64_t physical_address = memory_start;
             physical_address < memory_end; physical_address += PAGE_SIZE) {
            VirtualAddress virtual_address = {0};
            virtual_address.value = physical_address;

            PageTableEntryFlags flags = {0};
            // TODO: different flags for different sections
            flags.execute = true;
            flags.read = true;
            flags.write = true;

            map_address(kernel_table, virtual_address, physical_address, flags);
        }

        // print_PageTable(kernel_table, true);

        set_active_PageTable(kernel_table);
    }

    // start processes
    {
        // simple process that gets data from register s1
        Process* p0 = allocate_process((uint64_t)process_load_s1);
        p0->context.s1 = 0x11111111;

        // process that gets data from its kernel stack
        Process* p1 = allocate_process((uint64_t)process_load_from_stack);
        // allocate on kernel stack so process doesn't clobber it
        // (though data will never be popped)
        p1->context.sp -= sizeof(SomeData);
        // put data on stack
        SomeData* p1_data = (SomeData*)(p1->context.sp);
        p1_data->d = 0xdeadbeef;
        p1_data->w = 0xfeedcafe;
        memcpy(p1_data->b, "abcdefghijklmno", 16);
        // p1_data->b[15] = 0;
        // pass address to data as s1
        p1->context.s1 = (uint64_t)p1_data;

        // process the eventually returns
        allocate_process((uint64_t)process_that_returns);

        allocate_process((uint64_t)process_mem_ops);
    }

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
