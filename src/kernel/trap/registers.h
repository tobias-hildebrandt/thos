#pragma once

#include <stdbool.h>

#include "bits.h"  // IWYU pragma: keep
#include "csr.h"   // IWYU pragma: keep

void set_trap_vector(void);
void prepare_sstatus_for_return(bool return_to_kernel_mode);
void enable_interrupts(void);
void disable_traps_now(void);
void enable_traps_now(void);

// 12.1.1.3 Supervisor Interrupt (sip and sie) Registers
// trigger supervisor software interrupt
#define KERNEL_SOFTWARE_INTERRUPT() \
    csr_set_mask_sip(BIT_TO_INT(SIE_SIP_SOFTWARE_INTERRUPT))
