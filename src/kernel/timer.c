#include "timer.h"

#include <stdint.h>

#include "asm.h"
#include "bits.h"
#include "build_info.h"
#include "csr.h"
#include "flags.h"  // IWYU pragma: keep
#include "sbi.h"

// sets bits in sie to enable supervisor timer interrupts
void enable_timer_interrupts(void) {
    uintptr_t sie = csr_read_sie();
    BIT_SET(sie, SIE_ENABLE_TIMER_INTERRUPTS);
    csr_write_sie(sie);
}

uint64_t read_time(void) {
    uint64_t time;
    if (POINTER_BITS == 64) {
        if (USE_SBI_SET_TIMER) {
            ASM("rdtime %0\n" : "=r"(time));
        } else {
            time = csr_read_time();
        }
    } else {
        uint32_t low;
        uint32_t high;

        if (USE_SBI_SET_TIMER) {
            ASM("rdtime %[low]\n"
                "rdtimeh %[high]\n" : [low] "=r"(low),
                [high] "=r"(high));

        } else {
            low = csr_read_time();
            high = csr_read_timeh();
        }
        time = ((uint64_t)high << 32) | (uint64_t)low;
    }
    return time;
}

void write_time(uint64_t time) {
    if (USE_SBI_SET_TIMER) {
        // handles 64/32 bits itself
        sbi_set_timer(time);
    } else {
        if (POINTER_BITS == 64) {
            csr_write_stimecmp(time);
        } else {
            csr_write_stimecmp((uint32_t)(time & 0xffffffff));
            csr_write_stimecmph((uint32_t)((time >> 32) & 0xffffffff));
        }
    }
}

// (unprivileged ISA) 7.1.1. "Zicntr" Extension for Base Counters and Timers
// unit is wall time clock ticks
void set_timer(uint64_t delay) {
    uint64_t time = read_time();
    time += delay;
    write_time(time);
}
