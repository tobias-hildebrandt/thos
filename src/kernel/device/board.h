#pragma once

#include <stdbool.h>
#include <stddef.h>

// TODO: wrap addresses in real types for better type safety?
struct Board {
    // Board name
    char* name;
    // Monitor harts (aka m-only cores)
    size_t num_monitor_harts;
    // Normal harts (aka application or user cores)
    size_t num_normal_harts;
    // SiFive test device
    void* sifive_test;
    // SiFive PLIC
    void* sifive_plic;
    // Sifive Uart 1
    void* sifive_uart1;
    // Does the board support CSR stimecmp writes?
    // else falls back to sbi set_timer
    bool csr_stimecmp;
    // Does the board support CSR time reads?
    // else falls back to rdtime
    bool csr_time;
};
typedef struct Board Board;

extern const Board board;

#define GET_BOARD_DEVICE(expr) board_check_valid_device((expr), #expr)

// use GET_BOARD_DEVICE macro
void* board_check_valid_device(void* device, char* str);
