#pragma once

#include <stdint.h>

#define CSR_READ_DECL(csr) uintptr_t csr_read_##csr(void)
#define CSR_WRITE_DECL(csr) uintptr_t csr_write_##csr(uintptr_t set)
#define CSR_SET_DECL(csr) uintptr_t csr_set_mask_##csr(uintptr_t mask)
#define CSR_UNSET_DECL(csr) uintptr_t csr_clear_mask_##csr(uintptr_t mask)
#define CSR_DECL(csr)    \
    CSR_READ_DECL(csr);  \
    CSR_WRITE_DECL(csr); \
    CSR_SET_DECL(csr);   \
    CSR_UNSET_DECL(csr);

// 12.1.1. Supervisor CSRs
CSR_DECL(sstatus)
CSR_DECL(stvec)
CSR_DECL(sip)
CSR_DECL(sie)
CSR_DECL(sscratch)
CSR_DECL(sepc)
CSR_DECL(scause)
CSR_DECL(stval)
CSR_DECL(satp)
CSR_DECL(stimecmp)
CSR_DECL(time)

// should only be used in 32bit
CSR_DECL(stimecmph)
CSR_DECL(timeh)

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
