#include "device/sifive_test.h"

#include <stdint.h>
#include <stdlib.h>

#include "device/board.h"
#include "flags.h"
#include "io.h"

enum {
    SIFIVE_TEST_SUCCESS = 0x5555,
    SIFIVE_TEST_FAILURE = 0x3333,
};

// writes exit code to board's sifive test device
void sifive_test_exit(int exit_code) {
    uint32_t* pointer = GET_BOARD_DEVICE(board.sifive_test);

    uint32_t encoded;
    if (exit_code == EXIT_SUCCESS) {
        encoded = SIFIVE_TEST_SUCCESS;
    } else {
        encoded = (exit_code << 16) | SIFIVE_TEST_FAILURE;
    };

    PRINTF_IF(DEBUG_EXIT, "exiting with code 0x%x\n", encoded);

    *pointer = encoded;
}
