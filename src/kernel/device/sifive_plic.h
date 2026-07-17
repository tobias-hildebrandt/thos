#pragma once

#include <stddef.h>
#include <stdint.h>

#define SIFIVE_PLIC_ADDRESS 0x0C000000UL
#define SIFIVE_PLIC_LEN 0x4000000

#define SIFIVE_PLIC_PRIORITY_OFFSET 0x0

/*
TODO: organize into struct? maybe generic device struct?
struct SifivePlicRegisterSection {
    size_t offset;
    size_t block_size;
};
*/

// each block 0x80
// for hart 0
// block of M mode enables
// for each hart [1,4]
// block of M mode enables
// block of S mode enables
#define SIFIVE_PLIC_INTERRUPT_ENABLE_OFFSET 0x00002000UL
#define SIFIVE_PLIC_INTERRUPT_ENABLE_BLOCK_SIZE 0x80

// each block 0x1000
// for hart 0
// block of m-mode prio + claim/complete
// for each hart [1,4]
// block of m-mode prio + claim/complete
// block of s-mode prio + claim/complete
#define SIFIVE_PLIC_THRESHOLD_CLAIM_COMPLETE_OFFSET 0x00200000UL
#define SIFIVE_PLIC_THRESHOLD_CLAIM_COMPLETE_BLOCK_SIZE 0x1000

void sifive_plic_enable_smode_interrupt(size_t normal_hart_number,
                                        size_t interrupt);
void sifive_plic_set_smode_threshold(size_t normal_hart_number,
                                     uint8_t threshold);
void sifive_plic_set_priority(size_t interrupt, uint8_t priority);
uint32_t sifive_plic_claim(size_t normal_hart_number);
void sifive_plic_complete(size_t normal_hart_number, uint32_t interrupt);
