#include "device/board.h"

#include <stdbool.h>
#include <stddef.h>

#include "device/sifive_plic.h"
#include "device/sifive_test.h"
#include "device/sifive_uart.h"
#include "panic.h"
#include "util.h"

// TODO: parse from device tree at runtime

#ifndef MACHINE
#pragma message "MACHINE unset, default to unknown!"
#define MACHINE unknown
#endif

#define BOARD() BOARD_(MACHINE)
#define BOARD_(m) CONCAT_(board, m)

static UNUSED const Board board_unknown = {
    .name = "unknown",
    .num_monitor_harts = 0,
    .num_normal_harts = 1,
};

static UNUSED const Board board_virt = {
    .name = "virt",
    .num_monitor_harts = 0,
    .num_normal_harts = 1,
    .sifive_test = (void*)SIFIVE_TEST_ADDRESS,
    .sifive_plic = (void*)SIFIVE_PLIC_ADDRESS,
    .sifive_uart1 = 0,
    .csr_stimecmp = true,
    .csr_time = true,
};
static UNUSED const Board board_sifive_u = {
    .name = "sifive_u",
    .num_monitor_harts = 1,
    .num_normal_harts = 4,
    .sifive_test = 0,
    .sifive_plic = (void*)SIFIVE_PLIC_ADDRESS,
    .sifive_uart1 = (void*)SIFIVE_UART1_ADDRESS,
};

const Board board = BOARD();

// use GET_BOARD_DEVICE macro
void* board_check_valid_device(void* device, char* field_str) {
    if (NULL == device) {
        PANIC("board panic: %s is not set for current board (%s)\n", field_str,
              board.name);
    }
    return device;
}
