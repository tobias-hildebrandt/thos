#include "timer.h"

#include <stdint.h>

#include "bits.h"
#include "build_info.h"
#include "csr.h"

// sets bits in sie to enable supervisor timer interrupts
void enable_timer_interrupts(void) {
    uintptr_t sie = csr_read_sie();
    BIT_SET(sie, SIE_ENABLE_TIMER_INTERRUPTS);
    csr_write_sie(sie);
}

// (unprivileged ISA) 7.1.1. "Zicntr" Extension for Base Counters and Timers
// unit is wall time clock ticks
void set_timer(uint64_t delay) {
    if (POINTER_BITS == 64) {
        // time is just one register
        uint64_t time = csr_read_time();

        time += delay;

        csr_write_stimecmp(time);
    } else if (POINTER_BITS == 32) {
        // time split over two registers on riscv32
        uint32_t low = csr_read_time();
        uint32_t high = csr_read_timeh();

        uint64_t time = ((uint64_t)high << 32) | (uint64_t)low;

        time += delay;

        csr_write_stimecmp((uint32_t)(time & 0xffffffff));
        csr_write_stimecmph((uint32_t)((time >> 32) & 0xffffffff));
    }
}
