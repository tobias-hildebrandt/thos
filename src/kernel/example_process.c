#include "example_process.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "asm.h"
#include "flags.h"
#include "io.h"
#include "paging.h"
#include "process.h"
#include "sections.h"

struct SomeData {
    uint64_t d;
    uint32_t w;
    uint8_t b[16];
};
typedef struct SomeData SomeData;

void print_SomeData(SomeData* data) {
    printf("SomeData { d: 0x%lx, w: 0x%x, b: %s }\n", data->d, data->w,
           data->b);
}

void spin(int loops) {
    for (int i = 0; i < loops; i++) {
        ASM("nop");
    }
}

// prints ID and some value (only if enabled)
void loop_print(uint64_t val) {
    PRINTF_IF(DEBUG_EXAMPLE_PROCESSES, "%u.%lx\n", my_pid(), val);
}

void print_process_start(const char* name) {
    printf("%s %u start\n", name, my_pid());
}

void process_loop(uint64_t start) {
    uint64_t counter = start;
    while (1) {
        counter += 1;
        loop_print(counter);
        spin(EXAMPLE_PROCESSES_SPIN);
        yield();
    }
}

// process that gets data from a0
void process_load_a0(uint64_t data) {
    print_process_start(__func__);
    printf("s1 data: 0x%lx\n", data);

    yield();

    process_loop(data);
}

// process that gets data from its stack, using a0 as the data pointer
void process_load_from_stack(SomeData* data) {
    print_process_start(__func__);

    printf("0x%lx = ", (uint64_t)data);

    print_SomeData(data);

    yield();

    process_loop(data->d);
}

// process that returns
void process_that_returns(void) {
    print_process_start(__func__);
    yield();

    for (int i = 0; i < 10; i++) {
        loop_print(i);
        spin(EXAMPLE_PROCESSES_SPIN);
        yield();
    }
}

// process that allocates its own page and read or writes memory each loop
void process_mem_ops(void) {
    print_process_start(__func__);

    uint8_t* page = (uint8_t*)alloc_page();

    printf("alloc'd page vaddr: %p\n", page);
    printf("alloc'd page paddr: %p\n",
           get_physical_address(my_page_table(),
                                (VirtualAddress){.value = (uint64_t)page}));

    yield();

    bool should_read = false;

    uint64_t sum = 0;
    while (1) {
        loop_print(sum);

        for (int i = 0; i < PAGE_SIZE; i++) {
            if (should_read) {
                // read
                sum += page[i];
            } else {
                // write
                page[i] = i;
            }
        }
        should_read = !should_read;
        yield();
    }
}

void start_example_processes(void) {
    Process* p0 = allocate_process((ProcessArguments){
        .entry_address = (uint64_t)process_load_a0,
        .is_user_program = false,
    });
    p0->context.a0 = 0x1111111111111111;

    Process* p1 = allocate_process((ProcessArguments){
        .entry_address = (uint64_t)process_load_from_stack,
        .is_user_program = false,
    });
    // allocate room on process's stack so process doesn't clobber it
    // (though this data will never be popped)
    p1->context.sp -= sizeof(SomeData);
    // put data on stack
    SomeData* p1_data = (SomeData*)(p1->context.sp);
    p1_data->d = 0xdeaddeaddeadbeef;
    p1_data->w = 0xfeedcafe;
    memcpy(p1_data->b, "abcdefghijklmno", 16);
    // pass address to data as a0
    p1->context.a0 = (uint64_t)p1_data;

    Process* p2 = allocate_process((ProcessArguments){
        .entry_address = (uint64_t)process_that_returns,
        .is_user_program = false,
    });
    (void)p2;

    Process* p3 = allocate_process((ProcessArguments){
        .entry_address = (uint64_t)process_mem_ops,
        .is_user_program = false,
    });
    (void)p3;

    Process* p4 = allocate_process((ProcessArguments){
        .entry_address = (uint64_t)USER_user_START,
        .is_user_program = true,
        .user_program_end = (uint64_t)USER_user_END,
    });
    (void)p4;

    Process* p5 = allocate_process((ProcessArguments){
        .entry_address = (uint64_t)USER_user2_START,
        .is_user_program = true,
        .user_program_end = (uint64_t)USER_user2_END,
    });
    (void)p5;
}
