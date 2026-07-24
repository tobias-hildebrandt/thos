#pragma once

#include <stdint.h>

#include "build_info.h"

#define SCAUSE(INTERRUPT, EXCEPTION) \
    ((INTERRUPT##UL << (POINTER_BITS - 1)) | (EXCEPTION##UL))
#define SCAUSE_CASE(INTERRUPT, EXCEPTION, STRING) \
    case SCAUSE(INTERRUPT, EXCEPTION):            \
        return STRING

#define SCAUSE_SUPERVISOR_SOFTWARE_INTERRUPT SCAUSE(1, 1)
#define SCAUSE_ECALL_U_MODE SCAUSE(0, 8)
#define SCAUSE_SUPERVISOR_TIMER_INTERRUPT SCAUSE(1, 5)
#define SCAUSE_SUPERVISOR_EXTERNAL_INTERRUPT SCAUSE(1, 9)

const char* decode_scause(uint64_t scause);
