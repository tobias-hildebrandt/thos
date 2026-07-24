#pragma once

#include "flags.h"
#include "lock.h"
#include "process/process.h"
#include "trap/frame.h"
#include "util.h"

void NORETURN kernel_switch(TrapFrame* frame);
void NORETURN jump_into_processes(void);

extern Process processes[PROCESSES_MAXIMUM];
extern SpinLock processes_lock;
