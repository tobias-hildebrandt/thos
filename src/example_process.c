#include "example_process.h"

#include "alloc.h"
#include "io.h"  // IWYU pragma: keep
#include "process.h"
#include "types.h"

// for longer spins, pass to make: CFLAGS_EXTRA="-DSPIN_TIMES=1024*1024*1024"
#ifndef SPIN_TIMES
#define SPIN_TIMES 1024
#endif

void print_SomeData(SomeData* data) {
    printf("SomeData { d: 0x%x, w: 0x%x, b: %s }\n", data->d, data->w, data->b);
}

void spin(int loops) {
    for (int i = 0; i < loops; i++) {
        __asm__ __volatile__("nop");
    }
}

// only print if told to by external define (make CFLAGS_EXTRA="-DLOOP_PRINT=1")
void loop_print(uint64_t val) {
#ifdef LOOP_PRINT
    printf("%d.%x ", my_pid(), val);
#else
    (void)val;
#endif
}

void print_process_start(char* name) {
    printf("%s %d start\n", name, my_pid());
}

void process_loop(uint64_t start) {
    uint64_t counter = start;
    while (1) {
        counter += 1;
        loop_print(counter);
        spin(SPIN_TIMES);
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
        spin(SPIN_TIMES);
        yield();
    }
}

// process that allocates its own page and read or writes memory each loop
void process_mem_ops() {
    print_process_start((char*)__func__);

    uint8_t* page = (uint8_t*)alloc_page();

    printf("page addr: 0x%x\n", (uint64_t)page);

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
