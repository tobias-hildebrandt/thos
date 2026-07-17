#include "paging.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "board.h"
#include "build_info.h"
#include "flags.h"
#include "io.h"
#include "panic.h"
#include "sections.h"
#include "sifive_plic.h"
#include "util.h"

#if POINTER_BITS == 64
#define PAGE_TABLE_TOP_LEVEL 2
#define VIRTUAL_ADDRESS_LEVEL_ENRTY_NUMBERS_ARRAY(ADDR) \
    {                                                   \
        ADDR.level0_entry_number,                       \
        ADDR.level1_entry_number,                       \
        ADDR.level2_entry_number,                       \
    }
#else
#define PAGE_TABLE_TOP_LEVEL 1
#define VIRTUAL_ADDRESS_LEVEL_ENRTY_NUMBERS_ARRAY(ADDR) \
    {                                                   \
        ADDR.level0_entry_number,                       \
        ADDR.level1_entry_number,                       \
    }
#endif

static uintptr_t next_page_address = 0;

IN_GLOBAL_SPECIAL PageTable kernel_page_table = NULL;
IN_GLOBAL_SPECIAL SatpRegister kernel_page_satp = {0};

void* alloc_page(void) {
    if (next_page_address == 0) {
        next_page_address = (uintptr_t)PAGES_START;
    }

    if (next_page_address > (uintptr_t)PAGES_END - PAGE_SIZE) {
        PANIC("page allocation would overflow page memory!");
    }

    uintptr_t this_page = next_page_address;
    next_page_address += PAGE_SIZE;

    // wipe page
    memset((void*)this_page, 0, PAGE_SIZE);

    return (void*)this_page;
}

bool is_leaf_node(PageTableEntryFlags flags) {
    return (flags.read || flags.write || flags.execute);
}

PageTable get_linked_table(PageTableEntry entry) {
    if (false == entry.flags.valid) {
        PANIC("passed invalid entry to get_linked_table");
    }
    uintptr_t page_num = entry.physical_page_num;
    uintptr_t next_table_addr = page_num * PAGE_SIZE;

    return (PageTable)next_table_addr;
}

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

void PageTableEntryFlags_print(PageTableEntryFlags flags,
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
        if (is_leaf_node(flags)) {
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
void map_address(PageTable first_table, VirtualAddress virtual_address,
                 uintptr_t physical_address, PageTableEntryFlags flags) {
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

    uint16_t level_entry_number[PAGE_TABLE_TOP_LEVEL + 1] =
        VIRTUAL_ADDRESS_LEVEL_ENRTY_NUMBERS_ARRAY(virtual_address);

    PageTable current_table = first_table;
    PageTableEntry* entry = NULL;

    PRINTF_IF(DEBUG_MAP_ADDRESS, "(arg)   ");

    // work down from highest level to lowest level
    for (uint8_t level = PAGE_TABLE_TOP_LEVEL; /* breaks inside */; level--) {
        uint16_t entry_number = level_entry_number[level];

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
            uintptr_t new_page_addr = (uintptr_t)alloc_page();
            uintptr_t new_page_number = new_page_addr / PAGE_SIZE;

            PRINTF_IF(DEBUG_MAP_ADDRESS, "(alloc) ");

            // add to entry
            entry->physical_page_num = new_page_number;
            entry->flags.valid = true;

            // navigate to it
            current_table = (PageTable)new_page_addr;
        } else {
            PRINTF_IF(DEBUG_MAP_ADDRESS, "(found) ");
            // next level table already exists
            current_table = get_linked_table(*entry);
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

void PageTableEntry_print(PageTableEntry entry) {
    printf("Entry { flags: ");
    PageTableEntryFlags_print(entry.flags, true);
    printf(", physical_page_num: 0x%llx }\n", entry.physical_page_num);
}

struct PrintPageTableRecurse {
    bool recurse;
    uint8_t level;
};
typedef struct PrintPageTableRecurse PrintPageTableRecurse;

void PageTable_print(PageTable table, bool only_valid_entries,
                     PrintPageTableRecurse recurse) {
    for (size_t i = 0; i < (PAGE_SIZE / sizeof(uintptr_t)); i++) {
        PageTableEntry entry = table[i];

        if (only_valid_entries && !entry.flags.valid) {
            continue;
        }

        if (recurse.recurse) {
            printf("(level[%u]) ", recurse.level);
        }

        printf("PageTable %p entry[%u] = ", table, i);
        PageTableEntry_print(entry);

        if (recurse.recurse && !is_leaf_node(entry.flags)) {
            PrintPageTableRecurse next_recurse = recurse;
            next_recurse.level -= 1;
            PageTable_print(get_linked_table(entry), only_valid_entries,
                            next_recurse);
        }
    }
}

SatpRegister satp_from_page_table(PageTable table) {
    SatpRegister satp_register = {0};

    satp_register.mode = SATP_MODE;
    satp_register.physical_page_num = (uintptr_t)table / PAGE_SIZE;

    return satp_register;
}

// adds the global special page, which is outside of normal memory, to the page
// table at its physical address
void map_global_special_page(PageTable page_table, PageTableEntryFlags flags) {
    map_address(page_table, (VirtualAddress){.value = GLOBAL_SPECIAL_PAGE},
                GLOBAL_SPECIAL_PAGE, flags);
}

// map entire kernel address space
void init_kernel_page_table(void) {
    // TODO: mega/giga pages for different sections?

    kernel_page_table = alloc_page();

    // prepare kernel_page_satp
    kernel_page_satp = satp_from_page_table(kernel_page_table);

    for (uintptr_t physical_address = MEMORY_START;
         physical_address < MEMORY_END; physical_address += PAGE_SIZE) {
        // virtual address = physical address
        VirtualAddress virtual_address = {.value = physical_address};

        PageTableEntryFlags flags = {0};
        // TODO: different flags for different sections
        flags.execute = true;
        flags.read = true;
        flags.write = true;

        map_address(kernel_page_table, virtual_address, physical_address,
                    flags);
    }

    // map device addresses

    if (board.sifive_test) {
        PRINTF_IF(DEBUG_DEVICE_ADDRESSES, "mapping sifive_test address: %p\n",
                  board.sifive_test);
        map_address(kernel_page_table,
                    (VirtualAddress){.value = board.sifive_test},
                    board.sifive_test,
                    (PageTableEntryFlags){.read = true, .write = true});
    }
    if (board.sifive_plic) {
        PRINTF_IF(DEBUG_DEVICE_ADDRESSES,
                  "mapping sifive_plic addresses: %p to %p\n",
                  board.sifive_plic, board.sifive_plic + SIFIVE_PLIC_LEN);
        for (uintptr_t page = board.sifive_plic;
             page < (board.sifive_plic + SIFIVE_PLIC_LEN); page += PAGE_SIZE) {
            map_address(kernel_page_table, (VirtualAddress){.value = page},
                        page,
                        (PageTableEntryFlags){.read = true, .write = true});
        }
    }
    if (board.sifive_uart1) {
        PRINTF_IF(DEBUG_DEVICE_ADDRESSES, "mapping sifive_uart1 address: %p\n",
                  board.sifive_uart1);
        map_address(kernel_page_table,
                    (VirtualAddress){.value = board.sifive_uart1},
                    board.sifive_uart1,
                    (PageTableEntryFlags){.read = true, .write = true});
    }

    map_global_special_page(
        kernel_page_table,
        (PageTableEntryFlags){.read = true, .write = true, .execute = true});
}

// TODO: deduplicate with init_kernel_page_table
// map program address space
void init_user_program_page_table(PageTable page_table, uintptr_t start_virtual,
                                  uintptr_t start_physical,
                                  uintptr_t end_physical) {
    // virtual address starts at zero
    VirtualAddress virtual_address = {.value = start_virtual};

    for (uintptr_t physical_address = start_physical;
         physical_address < end_physical; physical_address += PAGE_SIZE) {
        PageTableEntryFlags flags = {0};
        // TODO: different flags for different sections?
        flags.execute = true;
        flags.read = true;
        flags.write = true;
        flags.user = true;

        map_address(page_table, virtual_address, physical_address, flags);

        virtual_address.value += PAGE_SIZE;
    }

    map_global_special_page(
        page_table, (PageTableEntryFlags){.read = true, .execute = true});

    // map user_special page
    map_address(page_table, (VirtualAddress){.value = USER_SPECIAL_PAGE},
                USER_SPECIAL_PAGE,
                (PageTableEntryFlags){
                    .read = true,
                    .execute = true,
                    .user = true,
                });
}

// walks page tables until leaf, then returns physical address at entry
// TODO: refactor into macro/function? deduplicate from map_address
uintptr_t get_physical_address(PageTable table,
                               VirtualAddress virtual_address) {
    uint16_t level_entry_number[PAGE_TABLE_TOP_LEVEL + 1] =
        VIRTUAL_ADDRESS_LEVEL_ENRTY_NUMBERS_ARRAY(virtual_address);

    PageTable current_table = table;
    PageTableEntry entry = {0};

    for (uint8_t level = PAGE_TABLE_TOP_LEVEL; /* breaks inside */; level--) {
        uintptr_t entry_number = level_entry_number[level];

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

        current_table = get_linked_table(entry);
    }

    return ((entry.physical_page_num * PAGE_SIZE) +
            virtual_address.page_offset);
}
