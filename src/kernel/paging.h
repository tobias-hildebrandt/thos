#pragma once

#include <stdint.h>

#define PAGE_SIZE 4096

#define RISCV_MODE_SV39 8L
#define RISCV_MODE_SV32 1

#if POINTER_BITS == 64
#define SATP_MODE RISCV_MODE_SV39
#else
#define SATP_MODE RISCV_MODE_SV32
#endif

union VirtualAddress {
    struct {
        uintptr_t page_offset : 12;
#if (POINTER_BITS == 64)
        uintptr_t level0_entry_number : 9;
        uintptr_t level1_entry_number : 9;
        uintptr_t level2_entry_number : 9;
        uintptr_t unused : 25;
#else
        uintptr_t level0_entry_number : 10;
        uintptr_t level1_entry_number : 10;
#endif
    } __attribute__((packed));
    uintptr_t value;
} __attribute__((packed));
typedef union VirtualAddress VirtualAddress;

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
#if (POINTER_BITS == 64)
        uintptr_t physical_page_num : 44;
        uintptr_t _reserved : 10;
#else
        uintptr_t physical_page_num : 22;
#endif
    } __attribute__((packed));
    uintptr_t value;
} __attribute__((packed));
typedef union PageTableEntry PageTableEntry;

// Supervisor Address Translation & Protection
union SatpRegister {
    struct {
#if (POINTER_BITS == 64)
        uintptr_t physical_page_num : 44;
        uintptr_t address_space_id : 16;
        uintptr_t mode : 4;
#else
        uintptr_t physical_page_num : 22;
        uintptr_t address_space_id : 9;
        uintptr_t mode : 1;
#endif
    } __attribute__((packed));
    uintptr_t value;
} __attribute__((packed));
typedef union SatpRegister SatpRegister;

// a PageTable is just an array of PageTableEntrys
typedef PageTableEntry* PageTable;

void* alloc_page(void);

extern PageTable kernel_page_table;
extern SatpRegister kernel_page_satp;
SatpRegister satp_from_page_table(PageTable table);
void init_kernel_page_table(void);
void init_user_program_page_table(PageTable page_table, uintptr_t start_virtual,
                                  uintptr_t start_physical,
                                  uintptr_t end_physical);
uintptr_t get_physical_address(PageTable table, VirtualAddress address);
