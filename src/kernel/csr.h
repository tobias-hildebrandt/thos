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
