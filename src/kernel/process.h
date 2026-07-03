#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "paging.h"

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

struct ProcessContext {
    // program counter, return address, stack pointer
    uint64_t ra, sp;
    // saved registers
    uint64_t s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;
    // program counter to return to
    uint64_t program_counter;
};
typedef struct ProcessContext ProcessContext;

#define KERNEL_STACK_SIZE 8192

struct Process {
    uint8_t id;
    ProcessState state;
    uint8_t kernel_stack[KERNEL_STACK_SIZE];
    ProcessContext context;
    PageTable page_table;
};
typedef struct Process Process;

struct ProcessArguments {
    uint64_t entry_address;
    bool is_user_program;
    uint64_t user_program_end;
};
typedef struct ProcessArguments ProcessArguments;

extern Process* current_process;

#include "trap.h"

void begin_processes(void);
void kernel_switch(TrapFrame* frame);
#define yield() KERNEL_SOFTWARE_INTERRUPT()
Process* allocate_process(ProcessArguments args);
uint8_t my_pid(void);
PageTable my_page_table(void);
void print_Process(Process* process);
void print_user_progs(void);
bool is_kernel_process(Process* process);
