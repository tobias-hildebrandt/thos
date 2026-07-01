#include "process.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "asm.h"
#include "paging.h"
#include "panic.h"
#include "util.h"

// TODO: change process->context.ra once it is switched into for the first time
//       so it doesn't loop infinitely once it returns

#define NUM_PROCESSES 16

static Process processes[NUM_PROCESSES] = {0};

#define PRINT_CONTEXT_REG(context, r) printf("\t" #r ": 0x%x,\n", context.r);

void print_ProcessState(ProcessState state) {
    switch (state) {
        case PROCESS_UNUSED:
            printf("UNUSED");
            break;
        case PROCESS_READY:
            printf("READY");
            break;
        case PROCESS_RUNNABLE:
            printf("RUNNABLE");
            break;
        case PROCESS_RUNNING:
            printf("RUNNING");
            break;
        default:
            PANIC("tried to print invalid ProcessState");
    }
}

void print_Process(Process* process) {
    printf("Process {\n\tpid: 0x%x,\n\tstate: ", process->id);
    print_ProcessState(process->state);
    printf(",\n");
    PRINT_CONTEXT_REG(process->context, ra);
    PRINT_CONTEXT_REG(process->context, sp);
    // PRINT_CONTEXT_REG(process->context, s0);
    // PRINT_CONTEXT_REG(process->context, s1);
    // PRINT_CONTEXT_REG(process->context, s2);
    // PRINT_CONTEXT_REG(process->context, s3);
    // PRINT_CONTEXT_REG(process->context, s4);
    // PRINT_CONTEXT_REG(process->context, s5);
    // PRINT_CONTEXT_REG(process->context, s6);
    // PRINT_CONTEXT_REG(process->context, s7);
    // PRINT_CONTEXT_REG(process->context, s8);
    // PRINT_CONTEXT_REG(process->context, s9);
    // PRINT_CONTEXT_REG(process->context, s10);
    // PRINT_CONTEXT_REG(process->context, s11);
    printf("}\n");
}

Process* allocate_process(uint64_t entry_address) {
    Process* process = NULL;
    for (int i = 0; i < NUM_PROCESSES; i++) {
        if (processes[i].state == PROCESS_UNUSED) {
            process = &processes[i];
            process->id = i;
            break;
        }
    }

    if (process == NULL) {
        PANIC("no more available processes");
    }

    // create page table and map kernel memory
    PageTable page_table = (PageTable)alloc_page();
    map_all_kernel_memory(page_table);

    process->state = PROCESS_READY;
    // when switched into, "returns" to the entry address
    process->context.ra = entry_address;
    // when switched into, stack pointer is set to "top" of processes's own
    // kernel stack
    process->context.sp = (uint64_t)&process->kernel_stack + KERNEL_STACK_SIZE;
    // TODO: s0 might be the frame pointer, maybe do something with it?

    process->page_table = page_table;

    printf("allocated ");
    print_Process(process);

    return process;
}

#define RESTORE_LABEL STRINGIFY(switch_context) "_restore"
#define REPLACE_RA_LABEL STRINGIFY(switch_context) "_replace_ra"
#define RESTORE_REPLACE_RA_LABEL \
    STRINGIFY(switch_context) "_restore_replaced_ra"
#define RETURN_LABEL STRINGIFY(switch_context) "_return"

#define S_OFFSET 2

__attribute__((naked))
__attribute__((aligned(4))) void switch_context(ProcessContext* old,
                                                ProcessContext* new,
                                                ProcessState* new_state) {
    __asm__ __volatile__(
        // clang-format off
        // don't store if old context is null (0)
        "beq a0, zero, " REPLACE_RA_LABEL "\n"

        // store registers in old ProcessContext*
        REGISTER_MEM(sd, a0, ra, 0)
        REGISTER_MEM(sd, a0, sp, 1)
        REGISTER_MEM_PREFIX(sd, a0, S_OFFSET, s, 0)
        REGISTER_MEM_PREFIX(sd, a0, S_OFFSET, s, 1)
        REGISTER_MEM_PREFIX(sd, a0, S_OFFSET, s, 2)
        REGISTER_MEM_PREFIX(sd, a0, S_OFFSET, s, 3)
        REGISTER_MEM_PREFIX(sd, a0, S_OFFSET, s, 4)
        REGISTER_MEM_PREFIX(sd, a0, S_OFFSET, s, 5)
        REGISTER_MEM_PREFIX(sd, a0, S_OFFSET, s, 6)
        REGISTER_MEM_PREFIX(sd, a0, S_OFFSET, s, 7)
        REGISTER_MEM_PREFIX(sd, a0, S_OFFSET, s, 8)
        REGISTER_MEM_PREFIX(sd, a0, S_OFFSET, s, 9)
        REGISTER_MEM_PREFIX(sd, a0, S_OFFSET, s, 10)
        REGISTER_MEM_PREFIX(sd, a0, S_OFFSET, s, 11)

        REPLACE_RA_LABEL":\n"

        // if new state is READY, replace RA with clean_process()
        // load old_state into t0
        "lw t0, (a2)\n"
        // load process ready state into t1
        "li t1, %[ready]\n"
        // skip down if state is not READY
        "bne t0, t1, " RESTORE_LABEL "\n"
        // load old ra to jump to later
        "ld t4, (a1)\n"
        // load clean_process into t2
        "la t2, " STRINGIFY(clean_process) "\n"
        // overwrite stack's ra
        "sd t2, (a1)\n"

        // load registers from new ProcessContext*
        RESTORE_LABEL":\n"

        // overwrite state with running
        "li t3, %[running]\n"
        "sw t3, 0(a2)\n"

        REGISTER_MEM(ld, a1, ra, 0)
        REGISTER_MEM(ld, a1, sp, 1)
        REGISTER_MEM_PREFIX(ld, a1, S_OFFSET, s, 0)
        REGISTER_MEM_PREFIX(ld, a1, S_OFFSET, s, 1)
        REGISTER_MEM_PREFIX(ld, a1, S_OFFSET, s, 2)
        REGISTER_MEM_PREFIX(ld, a1, S_OFFSET, s, 3)
        REGISTER_MEM_PREFIX(ld, a1, S_OFFSET, s, 4)
        REGISTER_MEM_PREFIX(ld, a1, S_OFFSET, s, 5)
        REGISTER_MEM_PREFIX(ld, a1, S_OFFSET, s, 6)
        REGISTER_MEM_PREFIX(ld, a1, S_OFFSET, s, 7)
        REGISTER_MEM_PREFIX(ld, a1, S_OFFSET, s, 8)
        REGISTER_MEM_PREFIX(ld, a1, S_OFFSET, s, 9)
        REGISTER_MEM_PREFIX(ld, a1, S_OFFSET, s, 10)
        REGISTER_MEM_PREFIX(ld, a1, S_OFFSET, s, 11)

        // if former state was not READY, go to return instruction
        "bne t0, t1, " RETURN_LABEL "\n"
        // else, jump to former ra
        "jalr zero, t4, 0\n"
        RETURN_LABEL":\n"
        "ret\n"

        // clang-format on
        :
        : [ready] "i"(PROCESS_READY), [running] "i"(PROCESS_RUNNING)
        : "memory");
}

static Process* current_process;

uint8_t my_pid(void) {
    if (current_process == NULL) {
        PANIC("my_pid called while no current process");
    }
    return current_process->id;
}

PageTable my_page_table(void) {
    if (current_process == NULL) {
        PANIC("my_page_table called while no current process");
    }
    return current_process->page_table;
}

uint8_t increment_loop_id(uint8_t id) {
    id += 1;
    if (id >= NUM_PROCESSES) {
        id = 0;
    }
    return id;
}

void yield(void) {
    // place current process as runnable
    uint8_t next_id;
    Process* next_process;
    if (current_process == NULL) {
        next_id = 0;
    } else {
        current_process->state = PROCESS_RUNNABLE;
        next_id = increment_loop_id(current_process->id);
    }
    const uint8_t STOP_ID = next_id;

    while (1) {
        next_process = &processes[next_id];
        if (next_process->state == PROCESS_RUNNABLE ||
            next_process->state == PROCESS_READY) {
            // found a runnable process
            break;
        }
        next_id = increment_loop_id(next_id);
        // don't loop around
        if (next_id == STOP_ID) {
            PANIC("no runnable processes");
        }
    }

    ProcessContext* previous_context = NULL;
    if (current_process != NULL) {
        previous_context = &(current_process->context);
    }

    current_process = next_process;
    // state is set in switch_context

    activate_PageTable(current_process->page_table);

    // will return INTO the process
    switch_context(previous_context, &(current_process->context),
                   &(current_process->state));
}

void clean_process(void) {
    printf("clean_process(pid:%d) ", my_pid());
    if (current_process == NULL) {
        PANIC("clean_process called but not current_process");
    }
    // wipe process
    memset(current_process, 0, sizeof(Process));
    current_process = NULL;

    // switch to some other process
    yield();
}
