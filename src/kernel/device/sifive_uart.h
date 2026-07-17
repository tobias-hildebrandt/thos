#pragma once

#include <stdint.h>  // IWYU pragma: keep

#include "flags.h"  // IWYU pragma: keep

// TODO: parse from device tree

#define SIFIVE_UART1_ADDRESS 0x10011000UL

// interrupt numbers
#define SIFIVE_UART0_INTERRUPT 4
#define SIFIVE_UART1_INTERRUPT 5

void sifive_uart_init(void);
void sifive_uart_drain(void);
