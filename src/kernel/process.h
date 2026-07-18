#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "buffer.h"
#include "paging.h"
#include "trap.h"

enum ProcessState {
    // not actually a process
    PROCESS_UNUSED = 0,
    // never yet run, but ready
    PROCESS_READY = 1,
    // already run some, but ready
    PROCESS_RUNNABLE,
    // currently running
    PROCESS_RUNNING,
};
typedef enum ProcessState ProcessState;

#define KERNEL_STACK_SIZE 8192

struct Process {
    uint8_t id;
    ProcessState state;
    uint8_t kernel_stack[KERNEL_STACK_SIZE];
    TrapFrame context;
    uintptr_t program_counter;
    PageTable page_table;
    char out_buffer_array[BUFFER_LEN];
    Buffer out_buffer;
};
typedef struct Process Process;

struct ProcessArguments {
    uintptr_t entry_address;
    bool is_user_program;
    uintptr_t user_program_end;
};
typedef struct ProcessArguments ProcessArguments;

extern void* current_process_stack_top;
extern Process* current_process;

void begin_processes(void);
void kernel_switch(TrapFrame* frame);
#define yield() KERNEL_SOFTWARE_INTERRUPT()
Process* allocate_process(ProcessArguments args);
uint8_t my_pid(void);
PageTable my_page_table(void);
void Process_print(Process* process);
void print_user_progs(void);
bool Process_is_kernel_process(Process* process);
void clean_process(void);
