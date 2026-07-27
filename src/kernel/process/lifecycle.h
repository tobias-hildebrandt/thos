#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "process/process.h"
#include "sections.h"

struct ProcessArguments {
    bool is_user_program;
    union {
        const Section* user_program_section;
        uintptr_t kernel_entry_address;
    };
};
typedef struct ProcessArguments ProcessArguments;

Process* Process_create(ProcessArguments args);
void Process_destroy_current(void);
