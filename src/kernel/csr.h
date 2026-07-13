#pragma once

#include <stdint.h>

#include "asm.h"

#define READ_CSR_IMPL(csr)                         \
    static inline uintptr_t csr_read_##csr(void) { \
        uintptr_t value;                           \
        ASM("csrr %0, " #csr "\n" : "=r"(value));  \
        return value;                              \
    }
#define WRITE_CSR_IMPL(csr)                               \
    static inline void csr_write_##csr(uintptr_t value) { \
        ASM("csrw " #csr ", %0\n" ::"r"(value));          \
    }
#define CSR_IMPL(csr)  \
    READ_CSR_IMPL(csr) \
    WRITE_CSR_IMPL(csr)

// 12.1.1. Supervisor CSRs
CSR_IMPL(sstatus)
CSR_IMPL(stvec)
CSR_IMPL(sip)
CSR_IMPL(sie)
CSR_IMPL(sepc)
CSR_IMPL(scause)
CSR_IMPL(stval)
CSR_IMPL(satp)
CSR_IMPL(stimecmp)
CSR_IMPL(time)

// should only be used in 32bit
CSR_IMPL(stimecmph)
CSR_IMPL(timeh)

// CSR bits

// SSTATUS.SIE supervisor interrupt (in general) enable
#define SSTATUS_TRAPS_NOW 2

// SSTATUS.SPIE supervisor traps will be enabled after sret (1) or disabled (0)
#define SSTATUS_TRAPS_AFTER_SRET 5

// SSTATUS.SPP privilege level to sret into, kernel mode (1) or user mode (0)
#define SSTATUS_PRIVILEGE 8

// SSTATUS.SUM allows kernel to access user-marked pages
#define SSTATUS_SUM 18

// SIP.SSIP supervisor software interrupt pending
#define SIP_SSIP 1

// SIE.STIE supervisor timer interrupt enable
#define SIE_ENABLE_TIMER_INTERRUPTS 5

// SIE.SSIE supervisor software interrupt enable
#define SIE_SSIE 1
