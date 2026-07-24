#include "csr.h"

#include <stdint.h>

#include "asm.h"
#include "build_info.h"

#define CSR_READ_IMPL(csr)                                  \
    inline CSR_READ_DECL(csr) {                             \
        uintptr_t value;                                    \
        ASM("csrr %[out], " #csr "\n" : [out] "=r"(value)); \
        return value;                                       \
    }
#define CSR_WRITE_IMPL(csr)                                 \
    inline CSR_WRITE_DECL(csr) {                            \
        uintptr_t out;                                      \
        ASM("csrrw %[out]," #csr ", %[set]\n" /*         */ \
            : [out] "=r"(out)                 /*         */ \
            : [set] "r"(set));                              \
        return out;                                         \
    }
#define CSR_SET_IMPL(csr)                                         \
    inline CSR_SET_DECL(csr) {                                    \
        uintptr_t out;                                            \
        ASM("csrrs %[out]," #csr ", %[mask]\n" /*              */ \
            : [out] "=r"(out)                  /*              */ \
            : [mask] "r"(mask));                                  \
        return out;                                               \
    }
#define CSR_UNSET_IMPL(csr)                                       \
    inline CSR_UNSET_DECL(csr) {                                  \
        uintptr_t out;                                            \
        ASM("csrrc %[out]," #csr ", %[mask]\n" /*              */ \
            : [out] "=r"(out)                  /*              */ \
            : [mask] "r"(mask));                                  \
        return out;                                               \
    }
#define CSR_IMPL(csr)   \
    CSR_READ_IMPL(csr)  \
    CSR_WRITE_IMPL(csr) \
    CSR_SET_IMPL(csr)   \
    CSR_UNSET_IMPL(csr)

// 12.1.1. Supervisor CSRs
CSR_IMPL(sstatus)
CSR_IMPL(stvec)
CSR_IMPL(sip)
CSR_IMPL(sie)
CSR_IMPL(sscratch)
CSR_IMPL(sepc)
CSR_IMPL(scause)
CSR_IMPL(stval)
CSR_IMPL(satp)
CSR_IMPL(stimecmp)
CSR_IMPL(time)

// can only be used in 32bit
#if POINTER_BITS == 32
CSR_IMPL(stimecmph)
CSR_IMPL(timeh)
#endif
