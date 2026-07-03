#include "process.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "asm.h"
#include "paging.h"
#include "panic.h"
#include "util.h"

#define NUM_PROCESSES 16

static Process processes[NUM_PROCESSES] = {0};

bool is_kernel_process(Process* process) {
    return process->page_table == kernel_page_table;
}

#define PRINT_CONTEXT_REG(context, r) \
    printf("\t" #r ": 0x%016lx,\n", context.r);

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
    printf("Process {\n");
    printf("\tpid: 0x%u,\n", process->id);
    printf("\tpage_table: (%s) %p\n",
           is_kernel_process(process) ? "KERNEL" : "USER", process->page_table);
    printf("\tstate: ");
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

// value for sstatus register, which controls sret behavior
//
// SPIE = 1 => supervisor traps enabled
// SPP  = 0 => switches processor to user mode
#define ENTER_USER_MODE_SSTATUS (1 << 5)

// entry user-memory address, must match what's defined in user program ld
// script
#define USER_PROGRAM_BASE 0x1000000

__attribute__((naked)) void enter_user_mode(uint64_t entry_point,
                                            uint64_t sstatus) {
    __asm__ __volatile__(
        // make sure sret jumps to entry_point
        "csrw sepc, a0\n"
        // set sret behavior
        "csrw sstatus, %[sstatus]\n"
        // "return" to sepc
        "sret\n"
        :
        : [sstatus] "r"((ENTER_USER_MODE_SSTATUS)));
}

Process* allocate_process(ProcessArguments args) {
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
    if (args.is_user_program) {
        PageTable page_table = (PageTable)alloc_page();
        // map process memory
        init_user_program_page_table(page_table, USER_PROGRAM_BASE,
                                     args.entry_address, args.user_program_end);

        process->page_table = page_table;
    } else {
        process->page_table = kernel_page_table;
    }

    process->state = PROCESS_READY;
    // when switched into, "returns" to the entry address
    process->context.ra = args.entry_address;
    // when switched into, stack pointer is set to "top" of processes's own
    // kernel stack
    process->context.sp = (uint64_t)&process->kernel_stack + KERNEL_STACK_SIZE;
    // TODO: s0 might be the frame pointer, maybe do something with it?

    printf("allocated ");
    print_Process(process);

    return process;
}

#define RESTORE_LABEL STRINGIFY(switch_context) "_restore"
#define REPLACE_RA_LABEL STRINGIFY(switch_context) "_replace_ra"
#define RETURN_LABEL STRINGIFY(switch_context) "_return"

// switch from old context to new context
// needs the state of the new process as an argument
__attribute__((naked)) __attribute__((aligned(4))) void switch_context(
    ProcessContext* old, ProcessContext* new, ProcessState* new_state) {
    // skip past storing if old context is null (0)
    ASM("beq a0, zero, " REPLACE_RA_LABEL "\n");

    // store registers in old ProcessContext*
    REGISTER_MEM(sd, a0, ra, ProcessContext);
    REGISTER_MEM(sd, a0, sp, ProcessContext);
    REGISTER_MEM(sd, a0, s0, ProcessContext);
    REGISTER_MEM(sd, a0, s1, ProcessContext);
    REGISTER_MEM(sd, a0, s2, ProcessContext);
    REGISTER_MEM(sd, a0, s3, ProcessContext);
    REGISTER_MEM(sd, a0, s4, ProcessContext);
    REGISTER_MEM(sd, a0, s5, ProcessContext);
    REGISTER_MEM(sd, a0, s6, ProcessContext);
    REGISTER_MEM(sd, a0, s7, ProcessContext);
    REGISTER_MEM(sd, a0, s8, ProcessContext);
    REGISTER_MEM(sd, a0, s9, ProcessContext);
    REGISTER_MEM(sd, a0, s10, ProcessContext);
    REGISTER_MEM(sd, a0, s11, ProcessContext);

    // TODO: figure out NAMED_REGISTER variables without clang uninit warning
    ASM(
        ASM_SET_LABEL(REPLACE_RA_LABEL)
        // if new state is READY, replace RA with clean_process()
        // load old_state
        "lw t0, %[ra_offset](a2)\n"
        // load process ready state into t1
        "li t1, %[ready]\n"
        // skip down if state is not READY
        "bne t0, t1, " RESTORE_LABEL "\n"
        // load old ra (the entry point) to jump to later
        "ld t4, %[ra_offset](a1)\n"
        // load clean_process into t2
        "la t2, " STRINGIFY(clean_process) "\n"
        // overwrite stack's ra
        "sd t2, %[ra_offset](a1)\n"
        ::
        [ra_offset] "i"(offsetof(ProcessContext, ra)),
        [ready] "i"(PROCESS_READY)
        : "memory"
    );

    ASM(ASM_SET_LABEL(RESTORE_LABEL)
        // overwrite state with running
        "li t3, %[running]\n"
        // write directly to new_state arg pointer
        // no offset since it's just an enum*, which is an int*
        "sw t3, 0(a2)\n"
        //
        ::[running] "i"(PROCESS_RUNNING));

    // load registers from new ProcessContext*
    REGISTER_MEM(ld, a1, ra, ProcessContext);
    REGISTER_MEM(ld, a1, sp, ProcessContext);
    REGISTER_MEM(ld, a1, s0, ProcessContext);
    REGISTER_MEM(ld, a1, s1, ProcessContext);
    REGISTER_MEM(ld, a1, s2, ProcessContext);
    REGISTER_MEM(ld, a1, s3, ProcessContext);
    REGISTER_MEM(ld, a1, s4, ProcessContext);
    REGISTER_MEM(ld, a1, s5, ProcessContext);
    REGISTER_MEM(ld, a1, s6, ProcessContext);
    REGISTER_MEM(ld, a1, s7, ProcessContext);
    REGISTER_MEM(ld, a1, s8, ProcessContext);
    REGISTER_MEM(ld, a1, s9, ProcessContext);
    REGISTER_MEM(ld, a1, s10, ProcessContext);
    REGISTER_MEM(ld, a1, s11, ProcessContext);

    // TODO: handle 4 cases
    // new kernel process
    // - dont set page table
    // - set sepc and sret
    // old kernel process
    // - dont set page table
    // - sret
    // new user process
    // - set page table
    // - set sepc and sret
    // old user process
    // - set page table
    // - sret

    ASM(/**/
        // if former state was not READY, go to return instruction
        "bne t0, t1, " RETURN_LABEL
        "\n"
        // else, jump to the entry point
        "jalr zero, t4, 0\n"

        RETURN_LABEL
        ":\n"
        "ret\n");
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

// TODO: make sure only way in is from a trap? then we are allowed to sret
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
    printf("clean_process(pid:%u) ", my_pid());
    if (current_process == NULL) {
        PANIC("clean_process called but not current_process");
    }
    // wipe process
    memset(current_process, 0, sizeof(Process));
    current_process = NULL;

    // switch to some other process
    yield();
}
