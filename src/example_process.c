#include "example_process.h"

#include "io.h"  // IWYU pragma: keep
#include "process.h"

#ifndef SPIN_TIMES
#define SPIN_TIMES (1024 /* * 1024 * 1024 */)
#endif

void print_SomeData(SomeData* data) {
    printf("SomeData {\n\td: 0x%x\n\tw: 0x%x\n\tb: %s\n}\n", data->d, data->w,
           data->b);
}

void spin(int loops) {
    for (int i = 0; i < loops; i++) {
        __asm__ __volatile__("nop");
    }
}

void process_loop(uint64_t start) {
    uint64_t counter = start;
    while (1) {
        counter += 1;
        printf("%d.%d ", my_pid(), counter);
        spin(SPIN_TIMES);
        yield();
    }
}

// process that gets data from s0
void process_load_s0() {
    uint64_t data;
    __asm__ __volatile__("mv %0, s0" : "=r"(data));

    printf("process %d start\n", my_pid());
    yield();

    process_loop(data);
}

// process that gets data from its kernel stack
void process_load_from_stack() {
    SomeData* data;
    __asm__ __volatile__("addi %[out], sp, %[size]"
                         : [out] "=r"(data)
                         : [size] "i"(sizeof(SomeData)));

    printf("process %d start\n", my_pid());

    print_SomeData(data);

    yield();

    process_loop(data->d);
}

// process that returns
void process_that_returns() {
    printf("process %d start\n", my_pid());
    yield();

    for (int i = 0; i < 10; i++) {
        printf("%d.%d ", my_pid(), i);
        spin(SPIN_TIMES);
        yield();
    }
}
