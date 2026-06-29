#pragma once

#include "types.h"  // IWYU pragma: keep

#define RISCV_MODE_SV39 8

union VirtualAddress {
    struct {
        uint64_t page_offset : 12;
        uint64_t level0_entry_number : 9;
        uint64_t level1_entry_number : 9;
        uint64_t level2_entry_number : 9;
        uint64_t unused : 25;
    } __attribute__((packed));
    uint64_t value;
} __attribute__((packed));
typedef union VirtualAddress VirtualAddress;

union PageTableEntryFlags {
    struct {
        uint64_t valid : 1;
        uint64_t read : 1;
        uint64_t write : 1;
        uint64_t execute : 1;
        uint64_t user : 1;
        uint64_t global : 1;
        uint64_t accessed : 1;
        uint64_t dirty : 1;
    } __attribute__((packed));
    uint8_t value;
} __attribute__((packed));
typedef union PageTableEntryFlags PageTableEntryFlags;

union PageTableEntry {
    struct {
        PageTableEntryFlags flags;
        uint64_t reserved_supervisor : 2;
        uint64_t physical_page_num : 44;
        uint64_t _reserved : 10;
    } __attribute__((packed));
    uint64_t value;
} __attribute__((packed));
typedef union PageTableEntry PageTableEntry;

// Supervisor Address Translation & Protection
union SatpRegister {
    struct {
        uint64_t physical_page_num : 44;
        uint64_t address_space_id : 16;
        uint64_t mode : 4;
    } __attribute__((packed));
    uint64_t value;
} __attribute__((packed));
typedef union SatpRegister SatpRegister;

// a PageTable is just an array of PageTableEntrys
typedef PageTableEntry* PageTable;

void map_address(PageTable first_table, VirtualAddress virtual_address,
                 uint64_t physical_address, PageTableEntryFlags flags);
void print_PageTable(PageTable table, bool only_valid_entries);
void set_active_PageTable(PageTable table);
