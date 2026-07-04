#include "process.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "asm.h"
#include "flags.h"
#include "io.h"
#include "paging.h"
#include "panic.h"
#include "sections.h"
#include "syscalls.h"
#include "trap.h"

// values for sstatus register, which controls sret behavior
//
// SPIE bit 5, supervisor traps enabled (1) or disabled (0)
// SPP  bit 8, kernel mode (1) or user mode (0)
#define SSTATUS_SUPERVISOR_TRAPS (1 << 5)
#define SSTATUS_USER_MODE (0)
#define SSTATUS_KERNEL_MODE (1 << 8)
#define SSTATUS_SUM (1 << 18)

// entry user-memory address
// must match what's defined in user program ld script
#define USER_PROGRAM_BASE 0x1000000

#define NUM_PROCESSES 16

static Process processes[NUM_PROCESSES];
Process* current_process = NULL;

// pointer to top of current_process's kernel stack
// kept up to date on switch
// makes trap_vector simpler
void* current_process_stack_top;

bool is_kernel_process(Process* process) {
    return process->page_table == kernel_page_table;
}

#define PRINT_CONTEXT_REG(context, r) \
    printf("\t" #r ": 0x%016lx,\n", context.r);

void print_ProcessState(ProcessState state) {
    printf("%d(", state);
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
    printf(")");
}

void print_Process(Process* process) {
    printf("Process {\n");
    printf("\tpid: 0x%u,\n", process->id);
    printf("\tpage_table: (%s) %p\n",
           process->page_table == 0     ? "NULL"
           : is_kernel_process(process) ? "KERNEL"
                                        : "USER",
           process->page_table);
    printf("\tstate: ");
    print_ProcessState(process->state);
    printf(",\n");
    PRINT_CONTEXT_REG(process->context, ra);
    if (process->page_table != NULL) {
        printf("\tra vaddr = paddr %p,\n",
               get_physical_address(
                   process->page_table,
                   (VirtualAddress){.value = process->context.ra}));
    }
    PRINT_CONTEXT_REG(process->context, sp);
    PRINT_CONTEXT_REG(process->context, pc);
    printf("}\n");
}

// user exit function
// located in user special function
// automatically set as return address when user process allocated
IN_USER_SPECIAL __attribute__((naked)) void user_exit(void) {
    ASM(
        // just do SYSCALL_EXIT and let kernel handle it
        "li a0, %0\n"
        "ecall\n"
        // fall through to fault if kernel doesn't clean
        "unimp\n" ::"i"(SYSCALL_EXIT));
}

Process* allocate_process(ProcessArguments args) {
    Process* process = NULL;
    int i = 0;
    for (; i < NUM_PROCESSES; i++) {
        if (processes[i].state == PROCESS_UNUSED) {
            process = &processes[i];
            break;
        }
    }

    // make sure we zero out everything including kernel stack
    memset(process, 0, sizeof(Process));

    // set ID
    process->id = i;

    if (process == NULL) {
        PANIC("no more available processes");
    }

    // create page table and map kernel memory
    if (args.is_user_program) {
        // needs page for page_table
        PageTable page_table = (PageTable)alloc_page();

        // TODO: copy into owned version
        // map process memory
        init_user_program_page_table(page_table, USER_PROGRAM_BASE,
                                     args.entry_address, args.user_program_end);

        process->page_table = page_table;

        // when switched into, "returns" to the virtual address base
        // of all user programs
        process->context.ra = USER_PROGRAM_BASE;

        // set stack pointer to user_program_end
        // TODO: maybe just leave it up to the user program?
        process->context.sp = args.user_program_end;
    } else {
        // uses normal kernel page table
        process->page_table = kernel_page_table;

        // needs room for stack
        uint64_t stack_page = (uint64_t)alloc_page();
        // points at top(!) of page
        process->context.sp = stack_page + PAGE_SIZE;

        // when switched into, "returns" to the entry address like normal
        process->context.ra = args.entry_address;
    }

    process->state = PROCESS_READY;
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

// calling this causes the kernel to start its processes
void begin_processes(void) {
    kernel_switch(NULL);
}

// wipes the current process
// TODO: de-alloc user page table once alloc/de-alloc is implemented
void clean_current_process(void) {
    printf("clean_process: pid %d\n", my_pid());
    if (current_process == NULL) {
        PANIC("clean_process called but no current_process");
    }

    // set process to unused
    // do wipe the kernel stack, we are currently on it!
    current_process->id = 0;
    current_process->state = PROCESS_UNUSED;

    printf("clean_process: wiped\n");

    current_process = NULL;
}

uint8_t next_id(uint8_t id) {
    return (id + 1) % NUM_PROCESSES;
}

Process* find_next_process(void) {
    // start at current id or 0
    uint8_t id = 0;
    if (current_process != NULL) {
        id = next_id(current_process->id);
    }

    int num_checked = 0;
    Process* process;
    while (1) {
        process = &processes[id];

        if (process->state == PROCESS_RUNNABLE ||
            process->state == PROCESS_READY) {
            // found a runnable process
            return process;
        }

        id = next_id(id);
        num_checked += 1;

        // only loop around once
        if (num_checked >= NUM_PROCESSES) {
            PANIC("no runnable or ready processes found after %d checks!",
                  num_checked);
        }
    }

    return process;
}

// switch to a different process
// called from inside a trap OR kernel_main OR clean_process
void kernel_switch(TrapFrame* frame) {
    PRINTF_IF(DEBUG_SWITCH, "kernel_switch: framepointer %p\n", frame);

    // back up previous process
    // can only happen if we started in trap_vector and didn't clean_up_process
    if (current_process != NULL && frame != NULL) {
        TrapFrame* context = &(current_process->context);
        // copy frame into old context
        memcpy(context, frame, sizeof(TrapFrame));

        current_process->state = PROCESS_RUNNABLE;

        // if user process, we have to add 4 to PC
        if (!is_kernel_process(current_process)) {
            current_process->context.pc += 4;
        }
        PRINTF_IF(DEBUG_SWITCH,
                  "kernel_switch: switch off pid %2d, %s, c.pc %p\n",
                  current_process->id,
                  is_kernel_process(current_process) ? "kernel" : "user  ",
                  context->pc);
        if (DEBUG_SWITCH == 2) {
            printf("kernel_switch: old context:\n");
            print_TrapFrame(context);
        }
    }

    Process* next_process = find_next_process();

    if (next_process == NULL) {
        PANIC("next_process NULL");
    }

    current_process = next_process;
    current_process_stack_top =
        &current_process->kernel_stack[KERNEL_STACK_SIZE];

    if (current_process->state == PROCESS_READY) {
        // first time this process is going to run

        // set sepc to return address (which is actually the entry point)
        ASM("csrw sepc, %0\n" ::"r"(current_process->context.ra));

        // set up new return address
        if (is_kernel_process(current_process)) {
            // TODO: implement
            current_process->context.ra = (uint64_t)0;
        } else {
            current_process->context.ra = (uint64_t)user_exit;
        }

    } else {
        // go back to restored counter
        ASM("csrw sepc, %0\n" ::"r"(current_process->context.pc));
    }

    // process now running
    current_process->state = PROCESS_RUNNING;

    if (DEBUG_SWITCH) {
        uint64_t sepc;
        ASM("csrr %0, sepc\n" : "=r"(sepc));
        printf("kernel_switch: switch on  pid %2d, %s, sepc %p\n",
               current_process->id,
               is_kernel_process(current_process) ? "kernel" : "user  ", sepc);
    }

    // TODO: memset 0 process's kernel stack?

    // set sret to go to kernel mode or user mode
    uint64_t sstatus;
    if (is_kernel_process(current_process)) {
        sstatus = SSTATUS_SUPERVISOR_TRAPS | SSTATUS_KERNEL_MODE | SSTATUS_SUM;
        ASM("csrw sstatus, %0\n" ::"r"(sstatus));
    } else {
        sstatus = SSTATUS_SUPERVISOR_TRAPS | SSTATUS_USER_MODE | SSTATUS_SUM;
        ASM("csrw sstatus, %0\n" ::"r"(sstatus));
    }

    // also make sure kernel traps are active
    enable_kernel_traps();

    SatpRegister new_satp = satp_from_page_table(current_process->page_table);

    if (DEBUG_SWITCH) {
        printf("kernel_switch: new page table %p, new satp: %p\n",
               current_process->page_table, new_satp.value);
    }
    if (DEBUG_SWITCH == 2) {
        printf("kernel_switch: new context:\n");
        print_TrapFrame(&current_process->context);
    }
    if (DEBUG_SWITCH) {
        printf("kernel_switch: switching now!\n");
    }

    current_process->context.satp = new_satp.value;

    restore_after_trap(&current_process->context);
}
