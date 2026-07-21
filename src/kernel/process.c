#include "process.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "asm.h"
#include "buffer.h"
#include "csr.h"
#include "flags.h"
#include "hart.h"
#include "io.h"
#include "lock.h"
#include "paging.h"
#include "panic.h"
#include "sections.h"
#include "syscalls.h"
#include "timer.h"
#include "trap.h"
#include "util.h"

// entry user-memory address
// must match what's defined in user program ld script
#define USER_PROGRAM_BASE 0x1000000UL

// TODO: move to flags.h
enum { NUM_PROCESSES = 64 };

static Process processes[NUM_PROCESSES];
static SpinLock processes_lock;

bool Process_is_kernel_process(Process* process) {
    return process->page_table == kernel_page_table;
}

static char* Process_type_str(Process* process) {
    if (Process_is_kernel_process(process)) {
        return "kernel";
    } else {
        return "user";
    }
}

#define PRINT_CONTEXT_REG(context, reg) \
    printf("\t" #reg ": 0x%p,\n", (context).reg);

static void ProcessState_print(ProcessState state) {
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

void Process_print(Process* process) {
    printf("Process {\n");
    printf("\tpid: 0x%u,\n", process->id);
    printf("\tpage_table: (%s) %p\n",
           process->page_table == 0 ? "NULL" : Process_type_str(process),
           process->page_table);
    printf("\tstate: ");
    ProcessState_print(process->state);
    printf(",\n");
    PRINT_CONTEXT_REG(process->frame, ra);
    if (process->page_table != NULL) {
        printf(
            "\tra vaddr = paddr %p,\n",
            get_physical_address(process->page_table,
                                 (VirtualAddress){.value = process->frame.ra}));
    }
    PRINT_CONTEXT_REG(process->frame, sp);
    PRINT_CONTEXT_REG(process->frame, pc);
    printf("}\n");
}

// user exit function
// located in user special function
// automatically set as return address when user process first switched on
static NORETURN IN_USER_SPECIAL NAKED void user_exit(void) {
    ASM(
        // just do SYSCALL_EXIT and let kernel handle it
        "li a0, %0\n"
        "ecall\n"
        // fall through to fault if kernel doesn't clean
        "unimp\n" ::"i"(SYSCALL_EXIT));
}

// kernel process exit
// automatically set as return address when kernel process first switched on
static NORETURN void kernel_exit(void) {
    disable_traps_now();

    // wipe current process (except kernel stack)
    clean_process();

    // switch into a new process
    reset_stack_begin_processes();
}

Process* allocate_process(ProcessArguments args) {
    Process* process = NULL;
    int index = 0;
    for (; index < NUM_PROCESSES; index++) {
        if (processes[index].state == PROCESS_UNUSED) {
            process = &processes[index];
            break;
        }
    }

    if (process == NULL) {
        PANIC("no more available processes");
    }

    // make sure we zero out everything including kernel stack
    memset(process, 0, sizeof(Process));

    // set ID
    process->id = index;

    // set up output buffer
    process->output_buffer = Buffer_wrap(process->out_buffer_array, BUFFER_LEN);

    // create page table and map memory
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
        process->frame.ra = USER_PROGRAM_BASE;

        // set stack pointer to user_program_end
        // TODO: maybe just leave it up to the user program?
        process->frame.sp = args.user_program_end;
    } else {
        // uses normal kernel page table
        process->page_table = kernel_page_table;

        // TODO: more stack space?
        // needs room for stack
        uintptr_t stack_page = (uintptr_t)alloc_page();
        // points at top(!) of page
        process->frame.sp = stack_page + PAGE_SIZE;

        // when switched into, "returns" to the entry address like normal
        process->frame.ra = args.entry_address;
    }

    process->state = PROCESS_READY;
    // TODO: s0 might be the frame pointer, maybe do something with it?

    if (DEBUG_ALLOCATE_PROCESS) {
        printf("allocated ");
        Process_print(process);
    }

    return process;
}

uint8_t my_pid(void) {
    Process* current_process = my_hart_current_process();
    // TODO: lock?
    if (current_process == NULL) {
        PANIC("my_pid called while no current process");
    }
    return current_process->id;
}

// Entry point into processes from kernel main and kernel_exit
// Precondition: traps disabled
void NAKED NORETURN reset_stack_begin_processes(void) {
    // clang-format off
    ASM(
        // reset (clobber) stack to top of work stack
        "csrr sp, sscratch\n"
        "li a0, %[offset]\n"
        "add sp, sp, a0\n"

        // switch into a new process
        "mv a0, x0\n"
        "j %[jump]\n"
        ::
        [offset]"i"(offsetof(HartScratch, work_stack)+ WORK_STACK_SIZE),
        [jump]"i"(kernel_switch));
    // clang-format on
}

// wipes the current process
// TODO: de-alloc user page table once alloc/de-alloc is implemented
// Precondition: traps disabled
void clean_process(void) {
    HartScratch* hart_scratch = my_hart_scratch();

    Process* old_process = hart_scratch->current_process;

    hart_scratch->current_process = NULL;

    PRINTF_IF(DEBUG_CLEAN_PROCESS, "clean_process: pid %d\n", my_pid());
    if (old_process == NULL) {
        PANIC("clean_process called but no current_process");
    }

    // set process to unused
    old_process->id = -1;
    old_process->state = PROCESS_UNUSED;

    // TODO: de-alloc page table, memory, etc

    PRINTF_IF(DEBUG_CLEAN_PROCESS, "clean_process: wiped\n");
}

static uint8_t next_id(uint8_t id) {
    return (id + 1) % NUM_PROCESSES;
}

// Precondition: process list locked
static Process* find_next_process(uint8_t start_id) {
    uint8_t id = start_id;

    Process* process;
    for (int num_checked = 0; num_checked < NUM_PROCESSES; num_checked++) {
        process = &processes[id];

        if (process->state == PROCESS_RUNNABLE ||
            process->state == PROCESS_READY) {
            // found a runnable process
            return process;
        }

        id = next_id(id);
    }
    return NULL;
}

// switch to a different process
// TODO: scheduling algorithm
// TODO: pass boolean, pull frame directly from hart scratch?
// Precondition: traps disabled
NORETURN void kernel_switch(TrapFrame* scratch_trap_frame) {
    HartScratch* scratch = my_hart_scratch();
    uintptr_t hart_id = my_hart_id();

    uint8_t start_search_id = 0;

    // back up previous process
    // can only happen if we started in trap_vector and didn't clean_up_process
    if (scratch->current_process != NULL && scratch_trap_frame != NULL) {
        TrapFrame* process_frame = &(scratch->current_process->frame);
        // copy frame into old context
        memcpy(process_frame, scratch_trap_frame, sizeof(TrapFrame));

        // if user process, we have to add 4 to PC
        if (!Process_is_kernel_process(scratch->current_process)) {
            scratch->current_process->frame.pc += 4;
        }
        PRINTF_IF(DEBUG_SWITCH,
                  "kernel_switch: hart 0x%x switch off pid %2d "
                  "(%-6s process), pc %p\n",
                  hart_id, scratch->current_process->id,
                  Process_is_kernel_process(scratch->current_process) ? "kernel"
                                                                      : "user",
                  process_frame->pc);
        if (DEBUG_SWITCH == 2) {
            printf("kernel_switch: hart 0x%x old context:\n", hart_id);
            TrapFrame_print(process_frame);
        }

        ASM("fence\n");

        start_search_id = next_id(scratch->current_process->id);

        // other harts can now choose this process
        scratch->current_process->state = PROCESS_RUNNABLE;

        scratch->current_process = NULL;
    }

    assert(scratch->current_process == NULL);

    bool process_first_run;

    while (1) {
        SpinLock_acquire(&processes_lock);

        Process* next_process = find_next_process(start_search_id);

        if (next_process == NULL) {
            SpinLock_release(&processes_lock);
            continue;
            // PANIC("next_process NULL");
        } else {
            scratch->current_process = next_process;

            process_first_run =
                (scratch->current_process->state == PROCESS_READY);

            // process now running, other harts will not try to choose this
            scratch->current_process->state = PROCESS_RUNNING;

            SpinLock_release(&processes_lock);
            break;
        }
    }

    // PC to sret into
    uintptr_t new_pc;

    if (process_first_run) {
        // first time this process is going to run

        // set sepc to return address (which is actually the entry point)
        new_pc = scratch->current_process->frame.ra;

        // set up new return address
        if (Process_is_kernel_process(scratch->current_process)) {
            scratch->current_process->frame.ra = (uintptr_t)kernel_exit;
        } else {
            scratch->current_process->frame.ra = (uintptr_t)user_exit;
        }
    } else {
        // go back to stored frame's PC
        new_pc = scratch->current_process->frame.pc;
    }

    if (DEBUG_SWITCH) {
        printf(
            "kernel_switch: hart 0x%x switch on  pid %2d "
            "(%-6s process), pc %p\n",
            hart_id, scratch->current_process->id,
            Process_is_kernel_process(scratch->current_process) ? "kernel"
                                                                : "user",
            new_pc);
    }

    csr_write_sepc(new_pc);

    // make sure kernel traps are active post-sret
    prepare_sstatus_for_return(
        Process_is_kernel_process(scratch->current_process));

    // enable necessary supervisor interrupts (will only actually be active
    // after return)
    enable_interrupts();

    SatpRegister new_satp =
        satp_from_page_table(scratch->current_process->page_table);

    if (DEBUG_SWITCH == 2) {
        printf("kernel_switch: hart 0x%x new page table %p, new satp: %p\n",
               hart_id, scratch->current_process->page_table, new_satp.value);
    }
    if (DEBUG_SWITCH == 2) {
        printf("kernel_switch: hart 0x%x new context:\n", hart_id);
        TrapFrame_print(&scratch->current_process->frame);
    }

    // TODO: just set it in allocate_process?
    scratch->current_process->frame.satp = new_satp.value;

    // copy process's trapframe into hart scratch's frame
    memcpy(&scratch->frame, &scratch->current_process->frame,
           sizeof(TrapFrame));

    // set interrupt timer
    set_timer(TIMER_INTERRUPT_DELAY);

    restore_after_trap(&scratch->frame);
}
