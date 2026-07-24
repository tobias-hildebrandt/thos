#pragma once

#include "trap/frame.h"
#include "util.h"

NORETURN void handle_trap(TrapFrame* frame);
