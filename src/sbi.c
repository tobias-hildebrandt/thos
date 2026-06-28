#include "sbi.h"

#include "types.h"

SbiReturn sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4,
                   long arg5, long fid, long eid) {
    // load the data into registers
    // (even though they should already be in the right place)
    REGISTER(a0) = arg0;
    REGISTER(a1) = arg1;
    REGISTER(a2) = arg2;
    REGISTER(a3) = arg3;
    REGISTER(a4) = arg4;
    REGISTER(a5) = arg5;
    REGISTER(a6) = fid;
    REGISTER(a7) = eid;

    // environment call
    __asm__ __volatile__("ecall"
                         // outputs
                         // TODO: maybe +r??
                         : "=r"(a0), "=r"(a1)
                         // inputs
                         : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5),
                           "r"(a6), "r"(a7)
                         // clobbers
                         : "memory");
    return (SbiReturn){.error = a0, .value = a1};
}

SbiReturn sbi_shutdown() {
    return sbi_call((uint32_t)0x0 /* shutdown */, (uint32_t)0x0 /* no reason */,
                    0, 0, 0, 0, 0x0, 0x53525354);
}
