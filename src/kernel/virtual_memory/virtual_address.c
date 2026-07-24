#include "virtual_memory/virtual_address.h"

#include <stdio.h>

#include "build_info.h"

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
