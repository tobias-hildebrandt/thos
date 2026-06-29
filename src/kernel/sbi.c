#include "sbi.h"

#include "asm.h"

SbiReturn __sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4,
                     long arg5, long fid, long eid) {
    // load the data into registers,
    // even though they should already be in the right place
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

// TODO:
// https://github.com/riscv-non-isa/riscv-sbi-doc/blob/master/src/ext-debug-console.adoc

// console putchar (legacy)
// https://github.com/riscv-non-isa/riscv-sbi-doc/blob/master/src/ext-legacy.adoc
SbiReturn sbi_putchar(char ch) {
    return SBI_CALL(0x1 /* Extension: Console Putchar */, 0x0 /* (ignored) */,
                    ch /* character */);
}

// system reset shutdown
// https://github.com/riscv-non-isa/riscv-sbi-doc/blob/master/src/ext-sys-reset.adoc
SbiReturn sbi_shutdown() {
    return SBI_CALL(0x53525354 /* System Reset Extension */,
                    0x0 /* Function: System reset */, 0x0 /* Shutdown */,
                    0x0 /* No reason */);
}
