#include "example_process.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "asm.h"
#include "flags.h"
#include "io.h"
#include "process/process.h"
#include "sections.h"
#include "virtual_memory/page.h"

struct SomeData {
    uint64_t d;
    uint32_t w;
    uint8_t b[16];
};
typedef struct SomeData SomeData;

static void SomeData_print(SomeData* data) {
    printf("SomeData { d: 0x%llx, w: 0x%x, b: %s }\n", data->d, data->w,
           data->b);
}

static void spin(int loops) {
    for (int i = 0; i < loops; i++) {
        ASM("nop");
    }
}

// prints ID and some value (only if enabled)
static void loop_print(uint64_t val) {
    PRINTF_IF(DEBUG_EXAMPLE_PROCESSES, "%u.%llx\n", my_pid(), val);
}

static void print_process_start(const char* name) {
    printf("%s %u start\n", name, my_pid());
}

static void process_loop(uint64_t start) {
    uint64_t counter = start;
    while (1) {
        counter += 1;
        loop_print(counter);
        spin(EXAMPLE_PROCESSES_SPIN);
        yield();
    }
}

// process that gets data from a0
static void process_load_a0(uint64_t data) {
    print_process_start(__func__);
    printf("s1 data: 0x%llx\n", data);

    yield();

    process_loop(data);
}

// process that gets data from its stack, using a0 as the data pointer
static void process_load_from_stack(SomeData* data) {
    print_process_start(__func__);

    printf("%p = ", (uintptr_t)data);

    SomeData_print(data);

    yield();

    process_loop(data->d);
}

// process that returns
static void process_that_returns(void) {
    print_process_start(__func__);
    yield();

    for (int i = 0; i < 10; i++) {
        loop_print(i);
        spin(EXAMPLE_PROCESSES_SPIN);
        yield();
    }
}

// process that allocates its own page and read or writes memory each loop
static void process_mem_ops(void) {
    print_process_start(__func__);

    uint8_t* page = (uint8_t*)Page_alloc();

    printf("alloc'd page vaddr: %p\n", page);

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

// process that never yields or SBI ecalls, to test timer interrupts
static void process_never_yields(void) {
    print_process_start(__func__);

    while (1) {
        spin(EXAMPLE_PROCESSES_SPIN);
    }
}

void start_example_processes(void) {
    // TODO: make a variety of processes up to NUM_PROCESSES

    for (int i = 0; i < 10; i++) {
        Process* proc_load_a0 = allocate_process((ProcessArguments){
            .entry_address = (uintptr_t)process_load_a0,
            .is_user_program = false,
        });
        proc_load_a0->frame.a0 = i;
    }

    Process* proc_load_stack = allocate_process((ProcessArguments){
        .entry_address = (uintptr_t)process_load_from_stack,
        .is_user_program = false,
    });
    // allocate room on process's stack so process doesn't clobber it
    // (though this data will never be popped)
    proc_load_stack->frame.sp -= sizeof(SomeData);
    // put data on stack
    // NOLINTNEXTLINE(performance-no-int-to-ptr)
    SomeData* stack_data = (SomeData*)(proc_load_stack->frame.sp);
    stack_data->d = 0xdeaddeaddeadbeef;
    stack_data->w = 0xfeedcafe;
    memcpy(stack_data->b, "abcdefghijklmno", 16);
    // pass address to data as a0
    proc_load_stack->frame.a0 = (uintptr_t)stack_data;

    Process* proc_returns = allocate_process((ProcessArguments){
        .entry_address = (uintptr_t)process_that_returns,
        .is_user_program = false,
    });
    (void)proc_returns;

    Process* proc_mem_ops = allocate_process((ProcessArguments){
        .entry_address = (uintptr_t)process_mem_ops,
        .is_user_program = false,
    });
    (void)proc_mem_ops;

    Process* proc_user_first = allocate_process((ProcessArguments){
        .entry_address = (uintptr_t)USER_first_START,
        .is_user_program = true,
        .user_program_end = (uintptr_t)USER_first_END,
    });
    (void)proc_user_first;

    Process* proc_user_second = allocate_process((ProcessArguments){
        .entry_address = (uintptr_t)USER_second_START,
        .is_user_program = true,
        .user_program_end = (uintptr_t)USER_second_END,
    });
    (void)proc_user_second;

    Process* proc_never_yields = allocate_process((ProcessArguments){
        .entry_address = (uintptr_t)process_never_yields,
        .is_user_program = false,
    });
    (void)proc_never_yields;
}
