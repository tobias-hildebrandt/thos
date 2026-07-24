#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "buffer.h"
#include "trap/frame.h"
#include "virtual_memory/page_table.h"

// entry user-memory address
// must match what's defined in user program ld script
#define USER_PROGRAM_BASE 0x1000000UL

enum ProcessState {
    // not actually a process
    PROCESS_UNUSED = 0,
    // never yet run, but ready
    PROCESS_READY = 1,
    // already run some, but ready
    PROCESS_RUNNABLE = 2,
    // currently running
    PROCESS_RUNNING = 3,
};
typedef enum ProcessState ProcessState;

struct Process {
    uint8_t id;
    ProcessState state;
    TrapFrame frame;
    PageTable page_table;
    char out_buffer_array[BUFFER_LEN];
    Buffer output_buffer;
};
typedef struct Process Process;

typedef uint8_t Pid;

#define yield() KERNEL_SOFTWARE_INTERRUPT()
Pid my_pid(void);
void Process_print(Process* process);
bool Process_is_kernel_process(Process* process);
