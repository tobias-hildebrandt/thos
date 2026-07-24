#pragma once

#include "util.h"

typedef NORETURN void(ProcessExit)(void);

ProcessExit ProcessExit_user;
ProcessExit ProcessExit_kernel;
