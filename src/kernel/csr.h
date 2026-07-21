#pragma once

#include <stdint.h>

#include "asm.h"

// TODO: CSR atomic read set or clear? csr rs / csr rc

#define CSR_READ_IMPL(csr)                                  \
    static inline uintptr_t csr_read_##csr(void) {          \
        uintptr_t value;                                    \
        ASM("csrr %[out], " #csr "\n" : [out] "=r"(value)); \
        return value;                                       \
    }
#define CSR_WRITE_IMPL(csr)                                  \
    static inline uintptr_t csr_write_##csr(uintptr_t set) { \
        uintptr_t out;                                       \
        ASM("csrrw %[out]," #csr ", %[set]\n" /*         */  \
            : [out] "=r"(out)                 /*         */  \
            : [set] "r"(set));                               \
        return out;                                          \
    }
#define CSR_SET_IMPL(csr)                                         \
    static inline uintptr_t csr_set_mask_##csr(uintptr_t mask) {  \
        uintptr_t out;                                            \
        ASM("csrrs %[out]," #csr ", %[mask]\n" /*              */ \
            : [out] "=r"(out)                  /*              */ \
            : [mask] "r"(mask));                                  \
        return out;                                               \
    }
#define CSR_UNSET_IMPL(csr)                                        \
    static inline uintptr_t csr_clear_mask_##csr(uintptr_t mask) { \
        uintptr_t out;                                             \
        ASM("csrrc %[out]," #csr ", %[mask]\n" /*              */  \
            : [out] "=r"(out)                  /*              */  \
            : [mask] "r"(mask));                                   \
        return out;                                                \
    }
#define CSR_IMPL(csr)   \
    CSR_READ_IMPL(csr)  \
    CSR_WRITE_IMPL(csr) \
    CSR_SET_IMPL(csr)   \
    CSR_UNSET_IMPL(csr)

// 12.1.1. Supervisor CSRs
CSR_IMPL(sstatus)
CSR_IMPL(stvec)
CSR_IMPL(sip)
CSR_IMPL(sie)
CSR_IMPL(sscratch)
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

// Bits in the SSTATUS register
enum {
    // SSTATUS.SIE
    // supervisor interrupt (in general) enable
    SSTATUS_TRAPS_NOW = 1,

    // SSTATUS.SPIE
    // supervisor traps will be enabled after sret (1) or disabled (0)
    SSTATUS_TRAPS_AFTER_SRET = 5,

    // SSTATUS.SPP
    // privilege level to sret into, kernel mode (1) or user mode (0)
    SSTATUS_PRIVILEGE = 8,

    // SSTATUS.SUM
    // allows kernel to access user-marked pages
    SSTATUS_SUM = 18,
};

// Bits in the SIE/SIP registers
enum {
    // SIE.SSIE/SIP.SSIP
    // supervisor software interrupt
    SIE_SIP_SOFTWARE_INTERRUPT = 1,

    // SIE.STIE/SIP.STIP
    // supervisor timer interrupt
    SIE_SIP_TIMER_INTERRUPT = 5,

    // SIE.SEIE/SIP.SEIP
    // supervisor external interrupt
    SIE_SIP_EXTERNAL_INTERRUPT = 9,
};
