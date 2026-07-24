#pragma once

#include "trap/frame.h"
#include "util.h"

void NORETURN restore_after_trap(TrapFrame* context);
void NORETURN trap_vector(void);
