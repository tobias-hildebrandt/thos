#include "sbi.h"

#include "asm.h"

SbiReturn __sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4,
                     long arg5, long fid, long eid) {
    // load the data into registers,
    // even though they should already be in the right place
    REGS_START
    REGISTER_INIT(a0, arg0);
    REGISTER_INIT(a1, arg1);
    REGISTER_INIT(a2, arg2);
    REGISTER_INIT(a3, arg3);
    REGISTER_INIT(a4, arg4);
    REGISTER_INIT(a5, arg5);
    REGISTER_INIT(a6, fid);
    REGISTER_INIT(a7, eid);

    // environment call
    ASM("ecall"
        // outputs
        // TODO: maybe +r??
        : "=r"(a0),
        "=r"(a1)
        // inputs
        : "r"(a0),
        "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5), "r"(a6),
        "r"(a7)
        // clobbers
        : "memory");
    return (SbiReturn){.error = a0, .value = a1};
    REGS_END
}

// TODO:
// https://github.com/riscv-non-isa/riscv-sbi-doc/blob/master/src/ext-debug-console.adoc

// console putchar (legacy)
// https://github.com/riscv-non-isa/riscv-sbi-doc/blob/master/src/ext-legacy.adoc
SbiReturn sbi_putchar(int ch) {
    return SBI_CALL(0x1 /* Extension: Console Putchar */, 0x0 /* (ignored) */,
                    ch /* character */);
}

// system reset shutdown
// https://github.com/riscv-non-isa/riscv-sbi-doc/blob/master/src/ext-sys-reset.adoc
SbiReturn sbi_shutdown(void) {
    return SBI_CALL(0x53525354 /* System Reset Extension */,
                    0x0 /* Function: System reset */, 0x0 /* Shutdown */,
                    0x0 /* No reason */);
}
