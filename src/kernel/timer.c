#include "timer.h"

#include <stdint.h>

#include "asm.h"
#include "build_info.h"
#include "csr.h"
#include "device/board.h"
#include "sbi.h"

static uint64_t read_time(void) {
    uint64_t time;
    if (POINTER_BITS == 64) {
        if (board.csr_time) {
            time = csr_read_time();
        } else {
            ASM("rdtime %0\n" : "=r"(time));
        }
    } else {
        uint32_t low;
        uint32_t high;

        if (board.csr_time) {
            low = csr_read_time();
            high = csr_read_timeh();
        } else {
            ASM("rdtime %[low]\n"
                "rdtimeh %[high]\n" : [low] "=r"(low),
                [high] "=r"(high));
        }
        time = ((uint64_t)high << 32) | (uint64_t)low;
    }
    return time;
}

static void write_time(uint64_t time) {
    if (board.csr_stimecmp) {
        if (POINTER_BITS == 64) {
            csr_write_stimecmp(time);
        } else {
            csr_write_stimecmp((uint32_t)(time & 0xffffffff));
            csr_write_stimecmph((uint32_t)((time >> 32) & 0xffffffff));
        }
    } else {
        // handles 64/32 bits itself
        sbi_set_timer(time);
    }
}

// (unprivileged ISA) 7.1.1. "Zicntr" Extension for Base Counters and Timers
// unit is wall time clock ticks
void set_timer(uint64_t delay) {
    uint64_t time = read_time();
    time += delay;
    write_time(time);
}
