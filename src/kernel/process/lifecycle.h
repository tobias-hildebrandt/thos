#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "process/process.h"

struct ProcessArguments {
    uintptr_t entry_address;
    bool is_user_program;
    uintptr_t user_program_end;
};
typedef struct ProcessArguments ProcessArguments;

Process* Process_create(ProcessArguments args);
void Process_destroy_current(void);
