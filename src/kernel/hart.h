#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "buffer.h"
#include "process/process.h"
#include "sections.h"
#include "trap/trap.h"

enum { WORK_STACK_SIZE = 8192 };

// TODO: move to flags.h
enum { HART_NUM_MAX = 16 };

// scratch space dedicated to a hart
// the CSR sscratch holds a pointer to this at all times
// (except briefly during traps)
struct HartScratch {
    // frame captured during trap
    TrapFrame frame;
    // process currently executing on hart (may be NULL)
    Process* current_process;
    // hart's output buffer, used during trapping/switching and init
    Buffer output_buffer;
    char output_array[BUFFER_LEN];
    // were interrupts enabled before locking?
    bool interrupts_before_locking;
    // lock depth
    uint32_t lock_depth;
    // hart's work stack
    // used during hart initialization and switching/interrupt handling
    uint8_t work_stack[WORK_STACK_SIZE];
};
typedef struct HartScratch HartScratch;

// array of hart scratch spaces
IN_GLOBAL_SPECIAL extern HartScratch hart_scratches[];

uintptr_t my_hart_id(void);
HartScratch* my_hart_scratch(void);
Process* my_hart_current_process(void);
Buffer* my_hart_or_process_output_buffer(bool were_interrupts_enabled);
void HartScratch_init_all(void);
bool my_hart_pre_lock_acquire(void);
void my_hart_post_lock_release(void);
