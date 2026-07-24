#include "trap/registers.h"

#include <stdbool.h>
#include <stdint.h>

#include "bits.h"
#include "csr.h"
#include "trap/routines.h"

// set up exception handler
void set_trap_vector(void) {
    // located in global special page
    // which is mapped for both kernel and user processes
    csr_write_stvec((uintptr_t)trap_vector);
}

// sets bits in sstatus to enable traps after return
void prepare_sstatus_for_return(bool return_to_kernel_mode) {
    uintptr_t sstatus = csr_read_sstatus();

    BIT_SET(sstatus, SSTATUS_TRAPS_AFTER_SRET);
    BIT_SET(sstatus, SSTATUS_SUM);
    BIT_BOOL(sstatus, SSTATUS_PRIVILEGE, return_to_kernel_mode);

    // printf("sstatus: %p\nsie: %p\n", sstatus, sie);

    // write
    csr_write_sstatus(sstatus);
}

// disable supervisor traps right now
void disable_traps_now(void) {
    csr_clear_mask_sstatus(BIT_TO_INT(SSTATUS_TRAPS_NOW));
}

// enable supervisor traps right now
void enable_traps_now(void) {
    csr_set_mask_sstatus(BIT_TO_INT(SSTATUS_TRAPS_NOW));
}

// enable SIE's supervisor software, timer, and external interrupt enable bits
void enable_interrupts(void) {
    uintptr_t sie = csr_read_sie();
    sie |= BIT_TO_INT(SIE_SIP_SOFTWARE_INTERRUPT) |
           BIT_TO_INT(SIE_SIP_TIMER_INTERRUPT) |
           BIT_TO_INT(SIE_SIP_EXTERNAL_INTERRUPT);
    csr_write_sie(sie);
}
