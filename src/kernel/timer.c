#include "timer.h"

#include <stdint.h>

#include "build_info.h"
#include "csr.h"

// STIE (supervisor timer interrupt enable) bit in SIE register
#define SIE_ENABLE_TIMER_INTERRUPTS (1 << 5)

// sets bits in sie to enable supervisor timer interrupts
void enable_timer_interrupts(void) {
    uintptr_t sie = csr_read_sie();
    sie |= SIE_ENABLE_TIMER_INTERRUPTS;
    csr_write_sie(sie);
}

// (unprivileged ISA) 7.1.1. "Zicntr" Extension for Base Counters and Timers
// unit is wall time clock ticks
void set_timer(uint64_t delay) {
    if (POINTER_BITS == 64) {
        uint64_t time = csr_read_time();
        time += delay;

        csr_write_stimecmp(time);
    } else if (POINTER_BITS == 32) {
        // rdtime only reads the low bits on riscv32
        // need to read rdtimeh for high bits

        uint32_t high = csr_read_timeh();
        uint32_t low = csr_read_time();

        uint64_t time = ((uint64_t)high << 32) | (uint64_t)low;

        time += delay;

        low = (uint32_t)(time & 0xffffffff);
        high = (uint32_t)((time >> 32) & 0xffffffff);
        csr_write_stimecmp(low);
        csr_write_stimecmph(high);
    }
}
