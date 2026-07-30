#include "virtual_memory/page_table.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "align.h"
#include "asm.h"
#include "build_info.h"
#include "device/board.h"
#include "device/sifive_plic.h"
#include "flags.h"
#include "io.h"
#include "panic.h"
#include "sections.h"
#include "virtual_memory/page.h"
#include "virtual_memory/satp.h"
#include "virtual_memory/virtual_address.h"

IN_GLOBAL_SPECIAL PageTable kernel_page_table = NULL;

#if POINTER_BITS == 64
enum { VIRTUAL_MEMORY_TOP_LEVEL = 2 };
#else
enum { VIRTUAL_MEMORY_TOP_LEVEL = 1 };
#endif

static bool PageTableEntryFlags_is_leaf(PageTableEntryFlags flags) {
    return (flags.read || flags.write || flags.execute);
}

static PageTable PageTableEntry_get_linked(PageTableEntry entry) {
    if (false == entry.flags.valid) {
        PANIC("passed invalid entry to get_linked_table");
    }
    uintptr_t page_num = entry.physical_page_num;
    // NOLINTNEXTLINE(performance-no-int-to-ptr)
    void* next_table = (void*)(page_num * PAGE_SIZE);

    return (PageTable)next_table;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static void PageTableEntryFlags_print(PageTableEntryFlags flags,
                                      bool print_leafiness) {
    if (PAGE_TABLE_PRINT_ALL_FLAGS) {
        printf("v%ur%uw%ux%uu%ug%ua%ud%u", flags.valid, flags.read, flags.write,
               flags.execute, flags.user, flags.global, flags.accessed,
               flags.dirty);

    } else {
        PRINTF_IF(flags.valid, "v");
        PRINTF_IF(flags.read, "r");
        PRINTF_IF(flags.write, "w");
        PRINTF_IF(flags.execute, "x");
        PRINTF_IF(flags.user, "u");
        PRINTF_IF(flags.global, "g");
        PRINTF_IF(flags.accessed, "a");
        PRINTF_IF(flags.dirty, "d");
    }

    if (print_leafiness) {
        if (PageTableEntryFlags_is_leaf(flags)) {
            printf(" (leaf)");
        } else {
            printf(" (inner)");
        }
    }
}

// map a virtual page address to physical page address in a given PageTable
//
// both addresses must be page-aligned
//
// sets given flags
//
// guarantees creation of all 3 levels of pages (no mega/giga pages)
// TODO: investigate qemu `info mem` some pages not accessed
static void PageTable_map_address(PageTable first_table,
                                  VirtualAddress virtual_address,
                                  uintptr_t physical_address,
                                  PageTableEntryFlags flags) {
    if (DEBUG_MAP_ADDRESS) {
        printf("map_address(table @ %p, v:%p, p:%p, f:", (uintptr_t)first_table,
               virtual_address.value, physical_address, flags);
        PageTableEntryFlags_print(flags, false);
        printf(")\n");
    }

    if (!is_aligned(virtual_address.value, PAGE_SIZE)) {
        PANIC("virtual address not aligned %p", virtual_address.value);
    }

    if (!is_aligned(physical_address, PAGE_SIZE)) {
        PANIC("physical address not aligned %p", physical_address);
    }

    if (DEBUG_MAP_ADDRESS) {
        VirtualAddress_print(virtual_address);
    }

    PageTable current_table = first_table;
    PageTableEntry* entry = NULL;

    PRINTF_IF(DEBUG_MAP_ADDRESS, "(arg)   ");

    // work down from highest level to lowest level
    for (uint8_t level = VIRTUAL_MEMORY_TOP_LEVEL; /* breaks inside */;
         level--) {
        uint16_t entry_number =
            VirtualAddress_get_level_entry_number(&virtual_address, level);

        if (current_table == NULL) {
            PANIC("null PageTable at level %u", level);
        }

        PRINTF_IF(DEBUG_MAP_ADDRESS, "level %i = table @ %p. using entry %u\n",
                  level, (uintptr_t)current_table, entry_number);

        entry = &current_table[entry_number];

        // stop iterating at level 0
        if (level == 0) {
            break;
        }

        if (false == entry->flags.valid) {
            // if next level table has not been created
            // create it
            Page new_page = Page_alloc();
            uintptr_t new_page_number = ((uintptr_t)new_page) / PAGE_SIZE;

            PRINTF_IF(DEBUG_MAP_ADDRESS, "(alloc) ");

            // add to entry
            entry->physical_page_num = new_page_number;
            entry->flags.valid = true;

            // navigate to it
            current_table = (PageTable)new_page;
        } else {
            PRINTF_IF(DEBUG_MAP_ADDRESS, "(found) ");
            // next level table already exists
            current_table = PageTableEntry_get_linked(*entry);
        }
    }

    // we are at leaf node
    // TODO: assert so

    // set data in level 0 entry
    entry->physical_page_num = physical_address / PAGE_SIZE;
    entry->flags.value = flags.value;

    // make sure it's valid
    entry->flags.valid = true;
    entry->flags.read = true;  // all leaf pages are readable
}

static void PageTableEntry_print(PageTableEntry entry) {
    printf("Entry { flags: ");
    PageTableEntryFlags_print(entry.flags, true);
    printf(", physical_page_num: 0x%llx }\n", entry.physical_page_num);
}

struct PageTablePrintState {
    bool recurse;
    uint8_t level;
};
typedef struct PageTablePrintState PageTablePrintState;

static const PageTablePrintState PageTablePrintState_start = {
    .recurse = true, .level = VIRTUAL_MEMORY_TOP_LEVEL};

static void PageTable_print(PageTable table, bool only_valid_entries,
                            PageTablePrintState state) {
    for (size_t i = 0; i < (PAGE_SIZE / sizeof(uintptr_t)); i++) {
        PageTableEntry entry = table[i];

        if (only_valid_entries && !entry.flags.valid) {
            continue;
        }

        if (state.recurse) {
            printf("(level[%-3u]) ", state.level);
        }

        printf("PageTable %p entry[%-3u] = ", table, i);
        PageTableEntry_print(entry);

        if (state.recurse && !PageTableEntryFlags_is_leaf(entry.flags)) {
            PageTablePrintState next_recurse = state;
            next_recurse.level -= 1;
            PageTable_print(PageTableEntry_get_linked(entry),
                            only_valid_entries, next_recurse);
        }
    }
}

struct PageTablePrintPageAddressesState {
    VirtualAddress virtual_address;
    uint8_t level;
};
typedef struct PageTablePrintPageAddressesState
    PageTablePrintPageAddressesState;

static const PageTablePrintPageAddressesState
    PageTablePrintPageAddressesState_start = {
        .virtual_address = {.value = 0}, .level = VIRTUAL_MEMORY_TOP_LEVEL};

static void PageTable_print_page_addresses(
    PageTable table, PageTablePrintPageAddressesState state) {
    for (size_t i = 0; i < (PAGE_SIZE / sizeof(uintptr_t)); i++) {
        PageTableEntry entry = table[i];

        if (!entry.flags.valid) {
            continue;
        }

        VirtualAddress_set_level_entry_number(&state.virtual_address,
                                              state.level, i);

        if (state.level == 0) {
            uintptr_t physical_address = entry.physical_page_num * PAGE_SIZE;
            if (state.virtual_address.value == physical_address) {
                printf("vaddr %p => (transparent)\n",
                       state.virtual_address.value);
            } else {
                printf("vaddr %p => paddr %p\n", state.virtual_address.value,
                       physical_address);
            }
        } else {
            PageTablePrintPageAddressesState next_state = state;
            next_state.level -= 1;
            PageTable_print_page_addresses(PageTableEntry_get_linked(entry),
                                           next_state);
        }
    }
}

static void PageTable_map_section(PageTable page_table, const Section* section,
                                  VirtualAddress virtual_address_start,
                                  PageTableEntryFlags flags) {
    uintptr_t physical_address = section->start_address;
    VirtualAddress virtual_address = virtual_address_start;
    while (physical_address <= section->end_address) {
        PageTable_map_address(page_table, virtual_address, physical_address,
                              flags);

        physical_address += PAGE_SIZE;
        virtual_address.value += PAGE_SIZE;
    }
}

// map entire kernel address space
PageTable PageTable_kernel_init(void) {
    // TODO: mega/giga pages for different sections?

    if (kernel_page_table != NULL) {
        return kernel_page_table;
    }

    kernel_page_table = Page_alloc();

    // prepare kernel_page_satp
    kernel_page_satp = SatpRegister_from_PageTable(kernel_page_table);

    // map entire kernel program memory transparently
    PageTable_map_section(
        kernel_page_table, &SECTION_MEMORY,
        (VirtualAddress){.value = SECTION_MEMORY.start_address},
        (PageTableEntryFlags){.read = true, .write = true, .execute = true});

    // map device addresses

    if (board.sifive_test) {
        PRINTF_IF(DEBUG_DEVICE_ADDRESSES, "mapping sifive_test address: %p\n",
                  board.sifive_test);
        PageTable_map_address(
            kernel_page_table,
            (VirtualAddress){.value = (uintptr_t)board.sifive_test},
            (uintptr_t)board.sifive_test,
            (PageTableEntryFlags){.read = true, .write = true});
    }
    if (board.sifive_plic) {
        PRINTF_IF(
            DEBUG_DEVICE_ADDRESSES, "mapping sifive_plic addresses: %p to %p\n",
            board.sifive_plic, (char*)board.sifive_plic + SIFIVE_PLIC_LEN);
        for (uintptr_t page = (uintptr_t)board.sifive_plic;
             page < ((uintptr_t)board.sifive_plic + SIFIVE_PLIC_LEN);
             page += PAGE_SIZE) {
            PageTable_map_address(
                kernel_page_table, (VirtualAddress){.value = page}, page,
                (PageTableEntryFlags){.read = true, .write = true});
        }
    }
    if (board.sifive_uart1) {
        PRINTF_IF(DEBUG_DEVICE_ADDRESSES, "mapping sifive_uart1 address: %p\n",
                  board.sifive_uart1);
        PageTable_map_address(
            kernel_page_table,
            (VirtualAddress){.value = (uintptr_t)board.sifive_uart1},
            (uintptr_t)board.sifive_uart1,
            (PageTableEntryFlags){.read = true, .write = true});
    }

    if (DEBUG_PAGETABLE_KERNEL == 1) {
        PageTable_print_page_addresses(kernel_page_table,
                                       PageTablePrintPageAddressesState_start);
    } else if (DEBUG_PAGETABLE_KERNEL == 2) {
        PageTable_print(kernel_page_table, true, PageTablePrintState_start);
    }

    return kernel_page_table;
}

// TODO: deduplicate with init_kernel_page_table
// map program address space
PageTable PageTable_user_init(const Section* program_section,
                              VirtualAddress start_virtual) {
    // needs page for page_table
    PageTable page_table = (PageTable)Page_alloc();

    // map program section at start address
    PageTable_map_section(
        page_table, program_section, start_virtual,
        (PageTableEntryFlags){
            .read = true, .write = true, .execute = true, .user = true});

    // map special sections transparently
    PageTable_map_section(
        page_table, &SECTION_GLOBAL_SPECIAL,
        (VirtualAddress){.value = SECTION_GLOBAL_SPECIAL.start_address},
        (PageTableEntryFlags){.read = true, .write = true, .execute = true});
    PageTable_map_section(
        page_table, &SECTION_USER_SPECIAL,
        (VirtualAddress){.value = SECTION_USER_SPECIAL.start_address},
        (PageTableEntryFlags){.read = true, .write = true, .execute = true});

    if (DEBUG_PAGETABLE_USER == 1) {
        PageTable_print_page_addresses(page_table,
                                       PageTablePrintPageAddressesState_start);
    } else if (DEBUG_PAGETABLE_USER == 2) {
        PageTable_print(page_table, true, PageTablePrintState_start);
    }

    return page_table;
}

// walks page tables until leaf, then returns physical address at entry
// TODO: refactor into macro/function? deduplicate from map_address
uintptr_t PageTable_get_physical_address(PageTable table,
                                         VirtualAddress virtual_address) {
    PageTable current_table = table;
    PageTableEntry entry = {0};

    for (uint8_t level = VIRTUAL_MEMORY_TOP_LEVEL; /* breaks inside */;
         level--) {
        uintptr_t entry_number =
            VirtualAddress_get_level_entry_number(&virtual_address, level);

        if (current_table == NULL) {
            PANIC("null PageTable at level %u", level);
        }

        entry = current_table[entry_number];

        if (level == 0) {
            break;
        }

        if (false == entry.flags.valid) {
            PANIC("Invalid inner table at level %u", level);
        }

        current_table = PageTableEntry_get_linked(entry);
    }

    return ((entry.physical_page_num * PAGE_SIZE) +
            virtual_address.page_offset);
}

void PageTable_activate_kernel(void) {
    ASM("sfence.vma\n"
        "csrw satp, %[satp]\n"
        "sfence.vma\n" ::[satp] "r"(kernel_page_satp.value));
}
