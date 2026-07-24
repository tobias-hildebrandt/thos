#pragma once

#include <stdint.h>

#include "build_info.h"
#include "virtual_memory/page.h"
#include "virtual_memory/page_table.h"

enum VirtualMemoryMode {
    RISCV_MODE_SV39 = 8,
    RISCV_MODE_SV32 = 1,
};

#if POINTER_BITS == 64
enum { SATP_MODE = RISCV_MODE_SV39 };
enum { SATP_MODE_BITWIDTH = 4 };
enum { ADDRESS_SPACE_ID_BITWIDTH = 16 };
#else
enum { SATP_MODE = RISCV_MODE_SV32 };
enum { SATP_MODE_BITWIDTH = 1 };
enum { ADDRESS_SPACE_ID_BITWIDTH = 9 };
#endif

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

extern SatpRegister kernel_page_satp;

SatpRegister SatpRegister_from_PageTable(PageTable table);
