#pragma once

#include <stdbool.h>

#include "flags.h"
#include "lock.h"
#include "process/process.h"
#include "util.h"

void kernel_switch(void);
void NORETURN jump_into_processes(void);

extern Process processes[PROCESSES_MAXIMUM];
extern SpinLock processes_lock;
