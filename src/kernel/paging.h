#pragma once

#include <stdint.h>

#include "build_info.h"

enum { PAGE_SIZE = 4096 };

enum VirtualMemoryMode {
    RISCV_MODE_SV39 = 8,
    RISCV_MODE_SV32 = 1,
};

#if POINTER_BITS == 64
enum { SATP_MODE = RISCV_MODE_SV39 };
enum { LEVEL_ENTRY_NUMBER_BITWIDTH = 9 };
enum { PHYSICAL_PAGE_NUMBER_BITWIDTH = 44 };
enum { ADDRESS_SPACE_ID_BITWIDTH = 16 };
enum { SATP_MODE_BITWIDTH = 4 };
#else
enum { SATP_MODE = RISCV_MODE_SV32 };
enum { LEVEL_ENTRY_NUMBER_BITWIDTH = 10 };
enum { PHYSICAL_PAGE_NUMBER_BITWIDTH = 22 };
enum { ADDRESS_SPACE_ID_BITWIDTH = 9 };
enum { SATP_MODE_BITWIDTH = 1 };
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

// Supervisor Address Translation & Protection
union SatpRegister {
    struct {
        uintptr_t physical_page_num : PHYSICAL_PAGE_NUMBER_BITWIDTH;
        uintptr_t address_space_id : ADDRESS_SPACE_ID_BITWIDTH;
        uintptr_t mode : SATP_MODE_BITWIDTH;
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

// TODO: call before jumping into switching code?
void activate_kernel_page_table(void);
