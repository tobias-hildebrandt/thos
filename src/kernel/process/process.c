#include "process/process.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "hart.h"
#include "panic.h"
#include "trap/registers.h"
#include "virtual_memory/page_table.h"
#include "virtual_memory/virtual_address.h"

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
        printf("\tra vaddr = paddr %p,\n",
               PageTable_get_physical_address(
                   process->page_table,
                   (VirtualAddress){.value = process->frame.ra}));
    }
    PRINT_CONTEXT_REG(process->frame, sp);
    PRINT_CONTEXT_REG(process->frame, pc);
    printf("}\n");
}

Pid my_pid(void) {
    Process* current_process = my_hart_current_process();
    // TODO: lock?
    if (current_process == NULL) {
        PANIC("my_pid called while no current process");
    }
    return current_process->id;
}

void yield(void) {
    trigger_supervisor_software_interrupt();
}
