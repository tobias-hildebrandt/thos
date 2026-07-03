#include "process.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "asm.h"
#include "flags.h"
#include "io.h"
#include "paging.h"
#include "panic.h"
#include "trap.h"

// values for sstatus register, which controls sret behavior
//
// SPIE bit 5, supervisor traps enabled (1) or disabled (0)
// SPP  bit 8, kernel mode (1) or user mode (0)
#define SSTATUS_SUPERVISOR_TRAPS (1 << 5)
#define SSTATUS_USER_MODE (0)
#define SSTATUS_KERNEL_MODE (1 << 8)

// entry user-memory address
// must match what's defined in user program ld script
#define USER_PROGRAM_BASE 0x1000000

#define NUM_PROCESSES 16

static Process processes[NUM_PROCESSES] = {0};
Process* current_process;

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

#define COPY_MEMBER(DEST, SOURCE, MEMBER) DEST->MEMBER = SOURCE->MEMBER

void copy_context_from_trap_frame(ProcessContext* context, TrapFrame* frame) {
    COPY_MEMBER(context, frame, ra);
    COPY_MEMBER(context, frame, sp);
    COPY_MEMBER(context, frame, s0);
    COPY_MEMBER(context, frame, s1);
    COPY_MEMBER(context, frame, s2);
    COPY_MEMBER(context, frame, s3);
    COPY_MEMBER(context, frame, s4);
    COPY_MEMBER(context, frame, s5);
    COPY_MEMBER(context, frame, s6);
    COPY_MEMBER(context, frame, s7);
    COPY_MEMBER(context, frame, s8);
    COPY_MEMBER(context, frame, s9);
    COPY_MEMBER(context, frame, s10);
    COPY_MEMBER(context, frame, s11);
}

// calling this causes the kernel to start its processes
void begin_processes(void) {
    kernel_switch(NULL);
}

void clean_process(void) {
    // TODO: fix

    // printf("clean_process(pid:%u) ", my_pid());
    // if (current_process == NULL) {
    //     PANIC("clean_process called but no current_process");
    // }
    // // wipe process
    // memset(current_process, 0, sizeof(Process));
    // current_process = NULL;

    // // TODO: make sure this works
    // // switch to some other process
    // begin_processes();
}

uint8_t increment_loop_id(uint8_t id) {
    id += 1;
    if (id >= NUM_PROCESSES) {
        id = 0;
    }
    return id;
}

Process* find_next_process(void) {
    uint8_t next_id = 0;
    if (current_process != NULL) {
        next_id = increment_loop_id(current_process->id);
    }
    const uint8_t STOP_ID = next_id;

    Process* next_process;
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

    return next_process;
}

// switch to a different process
// called from inside a trap OR kernel_main OR clean_process
void kernel_switch(TrapFrame* frame) {
    Process* next_process = find_next_process();

    if (next_process == NULL) {
        PANIC("next_process NULL");
    }

    if ((current_process == NULL && frame != NULL) ||
        (current_process != NULL && frame == NULL)) {
        PANIC(
            "kernel_switch: frame and current_process are not both NULL or "
            "non-NULL!");
    }

    // back up previous process (can only happen if we started in trap_vector)
    if (current_process != NULL && frame != NULL) {
        ProcessContext* context = &(current_process->context);
        // copy frame into old context
        copy_context_from_trap_frame(context, frame);

        // store sepc in context
        // do NOT add 4, it already points to the instruction after yield()
        uint64_t sepc;
        ASM("csrr %0, sepc\n" : "=r"(sepc));
        context->program_counter = sepc;
        current_process->state = PROCESS_RUNNABLE;
        PRINTF_IF(DEBUG_SWITCH, "kernel_switch: switch off pid %2d, pc   %p\n",
                  current_process->id, context->program_counter);
    }

    current_process = next_process;

    if (current_process->state == PROCESS_READY) {
        // first time this process is going to run

        // set sepc to return address (which is actually the entry point)
        ASM("csrw sepc, %0\n" ::"r"(current_process->context.ra));

        // TODO: fix process cleanup
        current_process->context.ra = (uint64_t)0;

    } else {
        // go back to restored counter
        ASM("csrw sepc, %0\n" ::"r"(current_process->context.program_counter));
    }

    // process now running
    current_process->state = PROCESS_RUNNING;

    if (DEBUG_SWITCH) {
        uint64_t sepc;
        ASM("csrr %0, sepc\n" : "=r"(sepc));
        printf("kernel_switch: switch on  pid %2d, sepc %p\n",
               current_process->id, sepc);
    }

    // check stack pointer
    if (current_process->context.sp < (uint64_t)current_process->kernel_stack ||
        current_process->context.sp >
            (uint64_t)&(current_process->kernel_stack[KERNEL_STACK_SIZE])) {
        PANIC(
            "process %d sp outside of kernel stack!\nsp:%p\nkernel stack: %p\n",
            current_process->context.sp, current_process->kernel_stack);
    }

    // set sret to go to kernel mode or user mode
    uint64_t sstatus;
    if (is_kernel_process(current_process)) {
        sstatus = SSTATUS_SUPERVISOR_TRAPS | SSTATUS_KERNEL_MODE;
        ASM("csrw sstatus, %0\n" ::"r"(sstatus));
    } else {
        sstatus = SSTATUS_SUPERVISOR_TRAPS | SSTATUS_USER_MODE;
        ASM("csrw sstatus, %0\n" ::"r"(sstatus));
    }

    // also make sure kernel traps are active
    enable_kernel_traps();

    SatpRegister new_satp = satp_from_page_table(current_process->page_table);

    restore_after_trap(&current_process->context, new_satp);
}
