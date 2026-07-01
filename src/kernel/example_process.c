#include "example_process.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "flags.h"
#include "io.h"  // IWYU pragma: keep
#include "paging.h"
#include "process.h"

struct SomeData {
    uint64_t d;
    uint32_t w;
    uint8_t b[16];
};
typedef struct SomeData SomeData;

void print_SomeData(SomeData* data) {
    printf("SomeData { d: 0x%x, w: 0x%x, b: %s }\n", data->d, data->w, data->b);
}

void spin(int loops) {
    for (int i = 0; i < loops; i++) {
        __asm__ __volatile__("nop");
    }
}

// prints ID and some value (only if enabled)
void loop_print(uint64_t val) {
    PRINTF_IF(DEBUG_EXAMPLE_PROCESSES, "%d.%x ", my_pid(), val);
}

void print_process_start(char* name) {
    printf("%s %d start\n", name, my_pid());
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

// process that gets data from s1
void process_load_s1() {
    uint64_t data;
    __asm__ __volatile__("mv %0, s1" : "=r"(data));

    print_process_start((char*)__func__);
    printf("s1 data: 0x%x\n", data);

    yield();

    process_loop(data);
}

// process that gets data from its kernel stack, using s1 as the data pointer
// TODO: add mechanism to set A registers in order to pass parameters
void process_load_from_stack() {
    SomeData* data;
    __asm__ __volatile__("mv %0, s1" : "=r"(data));
    print_process_start((char*)__func__);

    printf("0x%x = ", (uint64_t)data);

    print_SomeData(data);

    yield();

    process_loop(data->d);
}

// process that returns
void process_that_returns() {
    print_process_start((char*)__func__);
    yield();

    for (int i = 0; i < 10; i++) {
        loop_print(i);
        spin(EXAMPLE_PROCESSES_SPIN);
        yield();
    }
}

// process that allocates its own page and read or writes memory each loop
void process_mem_ops() {
    print_process_start((char*)__func__);

    uint8_t* page = (uint8_t*)alloc_page();

    printf("alloc'd page vaddr: 0x%x\n", (uint64_t)page);
    printf("alloc'd page paddr: 0x%x\n",
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
    Process* p0 = allocate_process((uint64_t)process_load_s1);
    p0->context.s1 = 0x11111111;

    Process* p1 = allocate_process((uint64_t)process_load_from_stack);
    // allocate room on kernel stack so process doesn't clobber it
    // (though this data will never be popped)
    p1->context.sp -= sizeof(SomeData);
    // put data on stack
    SomeData* p1_data = (SomeData*)(p1->context.sp);
    p1_data->d = 0xdeadbeef;
    p1_data->w = 0xfeedcafe;
    memcpy(p1_data->b, "abcdefghijklmno", 16);
    // pass address to data as s1
    p1->context.s1 = (uint64_t)p1_data;

    Process* p2 = allocate_process((uint64_t)process_that_returns);
    (void)p2;

    Process* p3 = allocate_process((uint64_t)process_mem_ops);
    (void)p3;
}
