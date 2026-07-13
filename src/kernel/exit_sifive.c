#include "exit_sifive.h"

#include <stdint.h>
#include <stdlib.h>

#include "asm.h"
#include "flags.h"
#include "io.h"

#define SIFIVE_TEST_SUCCESS 0x5555
#define SIFIVE_TEST_FAILURE 0x3333

// implement stdlib.h's exit
// writes exit code to qemu virt board's sifive test device
void exit(int exit_code) {
    uint32_t encoded;
    if (exit_code == EXIT_SUCCESS) {
        encoded = SIFIVE_TEST_SUCCESS;
    } else {
        encoded = (exit_code << 16) | SIFIVE_TEST_FAILURE;
    };

    PRINTF_IF(DEBUG_EXIT, "exiting with code 0x%x\n", encoded);

    while (1) {
        ASM("sw %[value], 0(%[test_device_address])\n"
            //
            ::[value] "r"(encoded),
            [test_device_address] "r"(SIFIVE_TEST_DEVICE_ADDR));
    }
}
