#include "virtual_memory/virtual_address.h"

#include <stdint.h>
#include <stdio.h>

#include "build_info.h"
#include "panic.h"

void VirtualAddress_print(VirtualAddress virtual_address) {
    printf("VirtualAddress(%p){ ", virtual_address.value);
#if POINTER_BITS == 64
    printf("L2: %u, ", virtual_address.level2_entry_number);
#endif
    printf("L1: %u, ", virtual_address.level1_entry_number);
    printf("L0: %u, ", virtual_address.level0_entry_number);
    printf("offset: %u ", virtual_address.page_offset);
    printf("}\n");
}

uint16_t VirtualAddress_get_level_entry_number(VirtualAddress* virtual_address,
                                               uintptr_t level) {
    if (level == 0) {
        return virtual_address->level0_entry_number;
    } else if (level == 1) {
        return virtual_address->level1_entry_number;
    }
#if POINTER_BITS == 64
    else if (level == 2) {
        return virtual_address->level2_entry_number;
    }
#endif
    else {
        PANIC("VirtualAddress_get_level invalid level %d", level);
    }
}
void VirtualAddress_set_level_entry_number(VirtualAddress* virtual_address,
                                           uintptr_t level,
                                           uintptr_t entry_number) {
    if (level == 0) {
        virtual_address->level0_entry_number = entry_number;
    } else if (level == 1) {
        virtual_address->level1_entry_number = entry_number;
    }
#if POINTER_BITS == 64
    else if (level == 2) {
        virtual_address->level2_entry_number = entry_number;
    }
#endif
    else {
        PANIC("VirtualAddress_set_level invalid level %d", level);
    }
}
