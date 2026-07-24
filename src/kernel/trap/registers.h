#pragma once

#include <stdbool.h>

#include "bits.h"  // IWYU pragma: keep
#include "csr.h"   // IWYU pragma: keep

void set_trap_vector(void);
void prepare_sstatus_for_return(bool return_to_kernel_mode);
void enable_interrupts(void);
void disable_traps_now(void);
void enable_traps_now(void);
void trigger_supervisor_software_interrupt(void);
