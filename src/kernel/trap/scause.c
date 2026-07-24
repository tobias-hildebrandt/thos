#include "trap/scause.h"

#include <stdint.h>

// 12.1.1.8. Supervisor Cause (scause) Register
const char* decode_scause(uint64_t scause) {
    switch (scause) {
            // clang-format off
        case SCAUSE_SUPERVISOR_SOFTWARE_INTERRUPT: return "Supervisor software interrupt";
        case SCAUSE_SUPERVISOR_TIMER_INTERRUPT: return "Supervisor timer interrupt";
        case SCAUSE_SUPERVISOR_EXTERNAL_INTERRUPT: return "Supervisor external interrupt";
        SCAUSE_CASE(1, 13, "Counter-overflow interrupt");
        SCAUSE_CASE(0, 0, "Instruction address misaligned");
        SCAUSE_CASE(0, 1, "Instruction access fault");
        SCAUSE_CASE(0, 2, "Illegal instruction");
        SCAUSE_CASE(0, 3, "Breakpoint");
        SCAUSE_CASE(0, 4, "Load address misaligned");
        SCAUSE_CASE(0, 5, "Load access fault");
        SCAUSE_CASE(0, 6, "Store/AMO address misaligned");
        SCAUSE_CASE(0, 7, "Store/AMO access fault");
        case SCAUSE_ECALL_U_MODE: return "Environment call from U-mode";
        SCAUSE_CASE(0, 9, "Environment call from S-mode");
        SCAUSE_CASE(0, 12, "Instruction page fault");
        SCAUSE_CASE(0, 13, "Load page fault");
        SCAUSE_CASE(0, 15, "Store/AMO page fault");
        SCAUSE_CASE(0, 18, "Software check");
        SCAUSE_CASE(0, 19, "Hardware error");
            // clang-format on
        default:
            return "(unknown)";
    }
}
