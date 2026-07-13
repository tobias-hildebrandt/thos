#include "timer.h"

#include <stdint.h>

#include "asm.h"
#include "build_info.h"

// sets bits in sie to enable supervisor timer interrupts
void enable_timer_interrupts(void) {
    uintptr_t sie;
    // read
    ASM("csrr %[sie], sie\n" : [sie] "=r"(sie));

    // STIE (supervisor timer interrupt enable) bit in SIE register
    sie |= 1 << 5;

    // write
    ASM("csrw sie, %[sie]\n" ::[sie] "r"(sie));
}

// (unprivileged ISA) 7.1.1. "Zicntr" Extension for Base Counters and Timers
// unit is wall time clock ticks
void set_timer(uint64_t delay) {
    if (POINTER_BITS == 64) {
        uint64_t time;
        ASM("rdtime %0\n" : "=r"(time));

        time += delay;

        ASM("csrw stimecmp, %0\n" ::"r"(time));
    } else if (POINTER_BITS == 32) {
        // rdtime only reads the low bits on riscv32
        // need to read rdtimeh for high bits

        uint32_t high;
        uint32_t low;
        ASM("rdtime %[low]\n"
            "rdtimeh %[high]\n" : [low] "=r"(low),
            [high] "=r"(high));

        uint64_t time = ((uint64_t)high << 32) | (uint64_t)low;

        time += delay;

        uint32_t low_new = (uint32_t)(time & 0xffffffff);
        uint32_t high_new = (uint32_t)((time >> 32) & 0xffffffff);
        ASM("csrw stimecmp, %[low]\n"
            "csrw stimecmph, %[high]\n"
            //
            ::[low] "r"(low_new),
            [high] "r"(high_new));
    }
}
