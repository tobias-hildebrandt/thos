#include "hart.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bits.h"
#include "buffer.h"
#include "csr.h"
#include "panic.h"
#include "process/process.h"
#include "sections.h"
#include "trap/registers.h"

// TODO:
// set up dynamically, first hart uses a dedicated space before starting others
IN_GLOBAL_SPECIAL HartScratch hart_scratches[HART_NUM_MAX];

uintptr_t my_hart_id(void) {
    uintptr_t sscratch = csr_read_sscratch();
    return (sscratch - (uintptr_t)&hart_scratches[0]) / sizeof(HartScratch);
}

HartScratch* my_hart_scratch(void) {
    // NOLINTNEXTLINE(performance-no-int-to-ptr)
    return (HartScratch*)csr_read_sscratch();
}

Process* my_hart_current_process(void) {
    return my_hart_scratch()->current_process;
}

Buffer* my_hart_or_process_output_buffer(bool were_interrupts_enabled) {
    HartScratch* hart_scratch = my_hart_scratch();

    if (hart_scratch->current_process != NULL && were_interrupts_enabled) {
        return &hart_scratch->current_process->output_buffer;
    } else {
        return &hart_scratch->output_buffer;
    }
}

void HartScratch_init_all(void) {
    for (size_t i = 0; i < HART_NUM_MAX; i++) {
        HartScratch* hart_scratch = &hart_scratches[i];
        hart_scratch->output_buffer =
            Buffer_wrap(hart_scratch->output_array, BUFFER_LEN);
    }
}

bool my_hart_pre_lock_acquire(void) {
    // disable traps and get old state
    bool were_interrupts_enabled =
        BIT_GET(csr_clear_mask_sstatus(BIT_TO_INT(SSTATUS_TRAPS_NOW)),
                SSTATUS_TRAPS_NOW);

    HartScratch* hart_scratch = my_hart_scratch();

    if (hart_scratch->lock_depth == 0) {
        hart_scratch->interrupts_before_locking = were_interrupts_enabled;
    }
    hart_scratch->lock_depth += 1;

    return were_interrupts_enabled;
}

void my_hart_post_lock_release(void) {
    HartScratch* hart_scratch = my_hart_scratch();

    if (hart_scratch->lock_depth == 0) {
        PANIC("post_lock_release lock_depth was 0");
    }

    hart_scratch->lock_depth -= 1;

    if (hart_scratch->lock_depth == 0 &&
        hart_scratch->interrupts_before_locking) {
        enable_traps_now();
    }
}
