#include "panic.h"

#include <stdint.h>
#include <stdlib.h>

#include "flags.h"
#include "hart.h"
#include "io.h"
#include "trap.h"
#include "util.h"

void _panic_start(void) {
    disable_traps_now();
    // TODO: redirect output buffer to hart-specific?
}

NORETURN void _panic_end(void) {
    if (PANIC_LOOP == 0) {
        exit(1);
    } else {
        while (1) {
        }
    }
}

void _panic_unused(UNUSED int unused, ...) {
}

NORETURN void _panic_safe(const char* file, const uint64_t line,
                          const char* func) {
    _panic_start();
    put_direct_str("!!!\n");
    put_direct_str("Kernel panic on hart ");
    put_direct_hex32(my_hart_id());
    put_direct_str("\n");
    put_direct_str(file);
    put_direct_str(":");
    put_direct_u32(line);
    put_direct_str("\nfunc ");
    put_direct_str(func);
    put_direct_str("\n!!!\n");
    _panic_end();
}
