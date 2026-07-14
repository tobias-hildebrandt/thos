#include <stdio.h>
#include <stdlib.h>

#include "flags.h"  // IWYU pragma: keep
#include "io.h"
#include "sbi.h"
#include "sifive_test.h"

// TODO: detect in qemu wrapper, shutdown qemu externally
#define SHUTDOWN_FAIL_STRING "---UNABLE TO SHUTDOWN---"

// implement's stdlib.h's exit
void exit(int exit_code) {
    PRINTF_IF(DEBUG_EXIT, "shutting down via %s\n",
              USE_SBI_EXIT ? "sbi" : "sifive_test");

    if (!USE_SBI_EXIT) {
        PRINTF_IF(DEBUG_EXIT, "trying sifive_test shutdown\n");
        sifive_test_exit(exit_code);
        PRINTF_IF(DEBUG_EXIT, "sifive_test_exit fail\n");
    }

    SbiReturn ret;

    printf("---EXITCODE %d---", exit_code);

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
