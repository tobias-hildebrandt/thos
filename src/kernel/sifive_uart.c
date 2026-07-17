#include "sifive_uart.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "asm.h"
#include "bits.h"
#include "board.h"
#include "flags.h"  // IWYU pragma: keep
#include "io.h"
#include "sifive_plic.h"

// registers are all 32bits and offset by number*32bits
#define SIFIVE_UART_REG(PTR, REGISTER) (PTR + REGISTER)
#define SIFIVE_UART_REG_TRANSMIT_DATA 0
#define SIFIVE_UART_REG_RECEIVE_DATA 1
#define SIFIVE_UART_REG_TRANSMIT_CONTROL 2
#define SIFIVE_UART_REG_RECEIVE_CONTROL 3
#define SIFIVE_UART_REG_INTERRUPT_ENABLE 4
#define SIFIVE_UART_REG_INTERRUPT_PENDING 5
#define SIFIVE_UART_REG_BAUD_DIVISOR 6

// bitfields for control registers

// enable
#define SIFIVE_UART_CONTROL_ENABLE 0
// triggers interrupt after FIFO holds at least this many words
#define SIFIVE_UART_CONTROL_INTERRUPT_THRESHOLD 16
#define SIFIVE_UART_CONTROL_INTERRUPT_THRESHOLD_LEN 3

// bitfields for interrupt registers
#define SIFIVE_UART_INTERRUPT_TRANSMIT 0
#define SIFIVE_UART_INTERRUPT_RECEIVE 1

// print register, skips "SIFIVE_UART_REG_"
#define SIFIVE_UART_PRINT_REG(BASE, REG)                                       \
    printf("%-18s @ %p = 0x%08x\n", (#REG) + (sizeof("SIFIVE_UART_REG_") - 1), \
           SIFIVE_UART_REG(BASE, REG), *SIFIVE_UART_REG(BASE, REG));

typedef uint32_t SifiveUartRegister;

// MUST read whole word, individual bytes will cause fault!
uint32_t sifive_uart_read_register(SifiveUartRegister* uart) {
    uint32_t value;
    ASM("lw %[value], (%[pointer])\n"
        //
        : [value] "=r"(value)
        //
        : [pointer] "r"(uart));
    return value;
}

void sifive_uart_print_regs(SifiveUartRegister* uart) {
    SIFIVE_UART_PRINT_REG(uart, SIFIVE_UART_REG_TRANSMIT_CONTROL);
    SIFIVE_UART_PRINT_REG(uart, SIFIVE_UART_REG_RECEIVE_CONTROL);
    SIFIVE_UART_PRINT_REG(uart, SIFIVE_UART_REG_INTERRUPT_ENABLE);
    SIFIVE_UART_PRINT_REG(uart, SIFIVE_UART_REG_INTERRUPT_PENDING);
    SIFIVE_UART_PRINT_REG(uart, SIFIVE_UART_REG_BAUD_DIVISOR);
}

void sifive_uart_init(void) {
    SifiveUartRegister* base = GET_BOARD_DEVICE(board.sifive_uart1);

    if (DEBUG_SIFIVE_UART) {
        printf("init_uart\n");

        printf("initial regs:\n");
        sifive_uart_print_regs(base);
    }

    // enable receive interrupt
    atomic_or_memory_word(
        SIFIVE_UART_REG(base, SIFIVE_UART_REG_INTERRUPT_ENABLE),
        BIT_TO_INT(SIFIVE_UART_INTERRUPT_RECEIVE));
    // enable receive control
    atomic_or_memory_word(
        SIFIVE_UART_REG(base, SIFIVE_UART_REG_RECEIVE_CONTROL),
        BIT_TO_INT(SIFIVE_UART_CONTROL_ENABLE));

    // TODO: set bits, to only interupt after X [0,7] chars in FIFO?
    // atomic_or_word(UART1, SIFIVE_UART_REG_RECEIVE_CONTROL,
    //                BIT_TO_INT(SIFIVE_UART_CONTROL_INTERRUPT_THRESHOLD));

    // set baud rate
    *(base + SIFIVE_UART_REG_BAUD_DIVISOR) = 16000;

    if (DEBUG_SIFIVE_UART) {
        printf("after regs set:\n");
        sifive_uart_print_regs(base);
    }

    // enable PLIC interrupt
    sifive_plic_enable_smode_interrupt(1, 5);

    // set priority threshold
    sifive_plic_set_smode_threshold(1, 0);

    // set uart1 interrupt priority
    sifive_plic_set_priority(5, 1);
}

// receive all pending characters
// TODO: do something else instead of printf-ing them?
void sifive_uart_drain(void) {
    SifiveUartRegister* pointer = GET_BOARD_DEVICE(board.sifive_uart1);

    SifiveUartRegister* receive = pointer + SIFIVE_UART_REG_RECEIVE_DATA;

    SifiveUartRegister* interrupt_pending =
        pointer + SIFIVE_UART_REG_INTERRUPT_PENDING;

    // TODO: max X characters to prevent lockout on continuous stream?

    // TODO: evaluate whether or not checking pending is actually correct?
    // TODO: maybe race conditions?

    bool pending;
    do {
        pending = BIT_GET(sifive_uart_read_register(interrupt_pending),
                          SIFIVE_UART_INTERRUPT_RECEIVE);
        if (pending) {
            uint32_t rx = sifive_uart_read_register(receive);
            // empty
            if (rx & (1 << 31)) {
                printf("SiFiveUart empty???\n");
            } else {
                char ch = (char)(rx & 0xff);

                PRINTF_IF(DEBUG_SIFIVE_UART, "SiFiveUart got char: 0x%02x\n",
                          ch);
                putchar(ch);
            }
        }
    } while (pending);
}
