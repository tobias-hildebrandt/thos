#pragma once

#include <stdint.h>

#include "build_info.h"

#if POINTER_BITS == 64
enum { LEVEL_ENTRY_NUMBER_BITWIDTH = 9 };
#else
enum { LEVEL_ENTRY_NUMBER_BITWIDTH = 10 };
#endif

union VirtualAddress {
    struct {
        uintptr_t page_offset : 12;
        uintptr_t level0_entry_number : LEVEL_ENTRY_NUMBER_BITWIDTH;
        uintptr_t level1_entry_number : LEVEL_ENTRY_NUMBER_BITWIDTH;
        ONLY64(uintptr_t level2_entry_number : LEVEL_ENTRY_NUMBER_BITWIDTH;)
        ONLY64(uintptr_t unused : 25;)
    } __attribute__((packed));
    uintptr_t value;
} __attribute__((packed));
typedef union VirtualAddress VirtualAddress;

void VirtualAddress_print(VirtualAddress virtual_address);
