#include <stdio.h>
#include <stdlib.h>

#include "device/board.h"
#include "device/sifive_test.h"
#include "flags.h"  // IWYU pragma: keep
#include "io.h"
#include "sbi.h"
#include "util.h"

// TODO: detect in qemu wrapper, shutdown qemu externally
#define SHUTDOWN_FAIL_STRING "---UNABLE TO SHUTDOWN---"

// implement's stdlib.h's exit
void NORETURN exit(int exit_code) {
    PRINTF_IF(DEBUG_EXIT, "shutting down via %s\n",
              board.sifive_test ? "sifive_test" : "sbi");

    if (board.sifive_test) {
        PRINTF_IF(DEBUG_EXIT, "trying sifive_test shutdown\n");
        sifive_test_exit(exit_code);
        PRINTF_IF(DEBUG_EXIT, "sifive_test_exit fail\n");
    }

    UNUSED SbiReturn ret;

    printf("---EXITCODE %d---\n", exit_code);

    PRINTF_IF(DEBUG_EXIT, "trying SBI shutdown\n");
    ret = sbi_shutdown(0x0);
    PRINTF_IF(DEBUG_EXIT, "SBI shutdown fail, error %ld\n", ret.error);

    PRINTF_IF(DEBUG_EXIT, "trying SBI reset\n");
    ret = sbi_shutdown(0x1);
    PRINTF_IF(DEBUG_EXIT, "SBI reset fail, error %ld\n", ret.error);

    printf("\n%s\n", SHUTDOWN_FAIL_STRING);
    while (1) {
    }
}
