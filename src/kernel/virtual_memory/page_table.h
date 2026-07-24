#pragma once

#include <stdint.h>

#include "build_info.h"
#include "virtual_memory/page.h"
#include "virtual_memory/virtual_address.h"

union PageTableEntryFlags {
    struct {
        uint8_t valid : 1;
        uint8_t read : 1;
        uint8_t write : 1;
        uint8_t execute : 1;
        uint8_t user : 1;
        uint8_t global : 1;
        uint8_t accessed : 1;
        uint8_t dirty : 1;
    } __attribute__((packed));
    uint8_t value;
} __attribute__((packed));
typedef union PageTableEntryFlags PageTableEntryFlags;

union PageTableEntry {
    struct {
        PageTableEntryFlags flags;
        uintptr_t reserved_supervisor : 2;
        uintptr_t physical_page_num : PHYSICAL_PAGE_NUMBER_BITWIDTH;
        ONLY64(uintptr_t _reserved : 10;)
    } __attribute__((packed));
    uintptr_t value;
} __attribute__((packed));
typedef union PageTableEntry PageTableEntry;

// a PageTable is just an array of PageTableEntrys
typedef PageTableEntry* PageTable;

extern PageTable kernel_page_table;

PageTable PageTable_kernel_init(void);
PageTable PageTable_user_init(uintptr_t start_virtual, uintptr_t start_physical,
                              uintptr_t end_physical);
uintptr_t PageTable_get_physical_address(PageTable table,
                                         VirtualAddress address);

void PageTable_activate_kernel(void);
