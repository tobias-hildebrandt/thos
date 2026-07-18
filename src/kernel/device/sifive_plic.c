#include "sifive_plic.h"

#include <stddef.h>
#include <stdint.h>

#include "asm.h"
#include "bits.h"
#include "device/board.h"
#include "flags.h"  // IWYU pragma: keep
#include "io.h"

typedef uint32_t SifivePlicRegister;

// return the block index of the s-mode block for a given normal hart
size_t normal_hart_s_mode_block_index(size_t normal_hart_number) {
    return
        // navigate past monitor hart blocks
        board.num_monitor_harts +
        // navigate past preceding normal harts' s and m mode blocks
        ((normal_hart_number - board.num_monitor_harts) * 2)
        // navigate past this hart's m mode block
        + 1;
}

void sifive_plic_enable_smode_interrupt(size_t normal_hart_number,
                                        size_t interrupt) {
    char* addr = GET_BOARD_DEVICE(board.sifive_plic);

    PRINTF_IF(DEBUG_SIFIVE_PLIC,
              "sifive_plic_enable_smode_interrupt: hart: %d, interrupt %d\n",
              normal_hart_number, interrupt);

    addr += SIFIVE_PLIC_INTERRUPT_ENABLE_OFFSET;
    addr += normal_hart_s_mode_block_index(normal_hart_number) *
            SIFIVE_PLIC_INTERRUPT_ENABLE_BLOCK_SIZE;

    // start at first register
    SifivePlicRegister* ptr = (void*)addr;

    // interrupts past 32 are in next register
    if (interrupt >= 32) {
        ptr += 1;
        interrupt -= 32;
    }

    atomic_or_memory_word(ptr, BIT_TO_INT(interrupt));
}

void sifive_plic_set_smode_threshold(size_t normal_hart_number,
                                     uint8_t threshold) {
    char* addr = GET_BOARD_DEVICE(board.sifive_plic);

    PRINTF_IF(DEBUG_SIFIVE_PLIC,
              "sifive_plic_set_smode_threshold: hart: %d, threshold %d\n",
              normal_hart_number, threshold);

    addr += SIFIVE_PLIC_THRESHOLD_CLAIM_COMPLETE_OFFSET;
    addr += normal_hart_s_mode_block_index(normal_hart_number) *
            SIFIVE_PLIC_THRESHOLD_CLAIM_COMPLETE_BLOCK_SIZE;

    // (threshold is first register in block)
    SifivePlicRegister* ptr = (void*)addr;

    // threshold must be in [0,7]
    threshold &= 0x7;
    *ptr = threshold;
}

void sifive_plic_set_priority(size_t interrupt, uint8_t priority) {
    char* addr = GET_BOARD_DEVICE(board.sifive_plic);

    PRINTF_IF(DEBUG_SIFIVE_PLIC,
              "sifive_plic_set_priority: interrupt: %d, priority %d\n",
              interrupt, priority);

    addr += SIFIVE_PLIC_PRIORITY_OFFSET;

    // one register per interrupt
    SifivePlicRegister* ptr = (void*)addr;
    ptr += interrupt;

    // priority must be in [0,7]
    priority &= 0x7;
    *ptr = priority;
}

uint32_t sifive_plic_claim(size_t normal_hart_number) {
    char* addr = GET_BOARD_DEVICE(board.sifive_plic);

    addr += SIFIVE_PLIC_THRESHOLD_CLAIM_COMPLETE_OFFSET;
    addr += normal_hart_s_mode_block_index(normal_hart_number) *
            SIFIVE_PLIC_THRESHOLD_CLAIM_COMPLETE_BLOCK_SIZE;

    // claim/completem is 2nd register in block
    SifivePlicRegister* ptr = (void*)addr;
    ptr += 1;

    return *ptr;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void sifive_plic_complete(size_t normal_hart_number, uint32_t interrupt) {
    char* addr = GET_BOARD_DEVICE(board.sifive_plic);

    addr += SIFIVE_PLIC_THRESHOLD_CLAIM_COMPLETE_OFFSET;
    addr += normal_hart_s_mode_block_index(normal_hart_number) *
            SIFIVE_PLIC_THRESHOLD_CLAIM_COMPLETE_BLOCK_SIZE;

    // claim/complete is 2nd register in block
    SifivePlicRegister* ptr = (void*)addr;
    ptr += 1;

    *ptr = interrupt;
}
