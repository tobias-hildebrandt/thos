#include "board.h"

#include <stdbool.h>
#include <stdint.h>

#include "panic.h"
#include "sifive_plic.h"
#include "sifive_test.h"
#include "sifive_uart.h"
#include "util.h"

// TODO: parse from device tree at runtime

#ifndef MACHINE
#define MACHINE unknown
#endif

#define BOARD() BOARD_(MACHINE)
#define BOARD_(m) CONCAT_(board, m)

const Board board_unknown = {
    .name = "unknown",
    .num_monitor_harts = 0,
    .num_normal_harts = 1,
};

const Board board_virt = {
    .name = "virt",
    .num_monitor_harts = 0,
    .num_normal_harts = 1,
    .sifive_test = SIFIVE_TEST_ADDRESS,
    .sifive_plic = SIFIVE_PLIC_ADDRESS,
    .sifive_uart1 = 0,
    .csr_stimecmp = true,
    .csr_time = true,
};
const Board board_sifive_u = {
    .name = "sifive_u",
    .num_monitor_harts = 1,
    .num_normal_harts = 4,
    .sifive_test = 0,
    .sifive_plic = SIFIVE_PLIC_ADDRESS,
    .sifive_uart1 = SIFIVE_UART1_ADDRESS,
};

const Board board = BOARD();

// use GET_BOARD_DEVICE macro
void* board_check_valid_device(uintptr_t value, char* field_str) {
    if (0 == value) {
        PANIC("board panic: %s is not set for current board (%s)\n", field_str,
              board.name);
    }
    return (void*)value;
}
