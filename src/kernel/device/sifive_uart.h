#pragma once

// TODO: parse from device tree

#define SIFIVE_UART1_ADDRESS 0x10011000UL

// interrupt numbers
enum {
    SIFIVE_UART0_INTERRUPT = 4,
    SIFIVE_UART1_INTERRUPT = 5,
};

void sifive_uart_init(void);
void sifive_uart_drain(void);
