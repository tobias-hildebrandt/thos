#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "buffer.h"
#include "trap/trap.h"
#include "util.h"
#include "virtual_memory/page_table.h"

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

struct ProcessArguments {
    uintptr_t entry_address;
    bool is_user_program;
    uintptr_t user_program_end;
};
typedef struct ProcessArguments ProcessArguments;

void NORETURN reset_stack_begin_processes(void);
void NORETURN kernel_switch(TrapFrame* frame);
#define yield() KERNEL_SOFTWARE_INTERRUPT()
Process* allocate_process(ProcessArguments args);
uint8_t my_pid(void);
void Process_print(Process* process);
void print_user_progs(void);
bool Process_is_kernel_process(Process* process);
void clean_process(void);
