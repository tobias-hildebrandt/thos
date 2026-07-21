#include "lock.h"

#include <stdbool.h>
#include <stdint.h>

#include "asm.h"
#include "flags.h"
#include "hart.h"
#include "io.h"

#define LOCK_INNER_AGAIN "_do_lock_lock_again"

// 13.1.4. "Zaamo" Extension for Atomic Memory Operations
// https://docs.riscv.org/reference/isa/v20260120/unpriv/a-st-ext.html#critical

bool SpinLock_acquire(SpinLock* lock) {
    bool were_interrupts_enabled = my_hart_pre_lock_acquire();

    uintptr_t former_value;
    // NOLINTNEXTLINE(bugprone-infinite-loop)
    do {
        if (lock->locked) {
            continue;
        }

        // clang-format off
        ASM("amoswap.w %[out], %[in], %[lock]\n"
            "fence r, rw\n"
            : [out] "=r"(former_value), [lock] "+A"(lock->locked)
            : [in] "r"(1U)
            : "memory");
        // clang-format on
    } while (former_value != 0);

    lock->hart_owner = my_hart_id();

    // TODO: real print
    if (DEBUG_SPINLOCK) {
        put_direct_str("SpinLock ");
        put_direct_hex32((uintptr_t)lock);
        put_direct_str(" acquired by hart ");
        put_direct_hex32(my_hart_id());
        put_direct_str("\n");
    }

    return were_interrupts_enabled;
}

void SpinLock_release(SpinLock* lock) {
    if (DEBUG_SPINLOCK) {
        put_direct_str("SpinLock ");
        put_direct_hex32((uintptr_t)lock);
        put_direct_str(" released by hart ");
        put_direct_hex32(my_hart_id());
        put_direct_str("\n");
    }

    lock->hart_owner = -1;

    ASM("fence rw, w\n"
        "amoswap.w x0, x0, %[addr]\n"
        "fence\n"
        //
        : [addr] "+A"(lock->locked) : : "memory");

    my_hart_post_lock_release();
}
