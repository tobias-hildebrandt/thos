#include "paging.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "flags.h"
#include "io.h"
#include "panic.h"
#include "process.h"
#include "sections.h"
#include "util.h"

static uint64_t next_page_address = 0;

void* alloc_page(void) {
    if (next_page_address == 0) {
        next_page_address = (uint64_t)__PAGES_START;
    }

    if (next_page_address > (uint64_t)__PAGES_END - PAGE_SIZE) {
        PANIC("page allocation would overflow page memory!");
    }

    uint64_t this_page = next_page_address;
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
    uint64_t page_num = entry.physical_page_num;
    uint64_t next_table_addr = page_num * PAGE_SIZE;

    return (PageTable)next_table_addr;
}

void print_VirtualAddress(VirtualAddress virtual_address) {
    printf("VirtualAddress(0x%x){ ", virtual_address.value);
    printf("L2: %d, ", virtual_address.level2_entry_number);
    printf("L1: %d, ", virtual_address.level1_entry_number);
    printf("L0: %d, ", virtual_address.level0_entry_number);
    printf("offset: %d ", virtual_address.page_offset);
    printf("}\n");
}

void print_PageTableEntryFlags(PageTableEntryFlags flags,
                               bool print_leafiness) {
    if (PAGE_TABLE_PRINT_ALL_FLAGS) {
        printf("v%dr%dw%dx%du%dg%da%dd%d", flags.valid, flags.read, flags.write,
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
                 uint64_t physical_address, PageTableEntryFlags flags) {
    if (DEBUG_MAP_ADDRESS) {
        printf("map_address(table @ 0x%x, v:0x%x, p:0x%x, f:",
               (uint64_t)first_table, virtual_address.value, physical_address,
               flags);
        print_PageTableEntryFlags(flags, false);
        printf(")\n");
    }

    if (!is_aligned(virtual_address.value, PAGE_SIZE)) {
        PANIC("virtual address not aligned %x", virtual_address.value);
    }

    if (!is_aligned(physical_address, PAGE_SIZE)) {
        PANIC("physical address not aligned %x", physical_address);
    }

    if (DEBUG_MAP_ADDRESS) {
        print_VirtualAddress(virtual_address);
    }

    uint64_t level_entry_number[] = {
        virtual_address.level0_entry_number,
        virtual_address.level1_entry_number,
        virtual_address.level2_entry_number,
    };
    PageTable current_table = first_table;
    PageTableEntry* entry = NULL;

    PRINTF_IF(DEBUG_MAP_ADDRESS, "(arg)   ");

    // work down from highest level to lowest level
    for (int level = 2; level >= 0; level--) {
        uint64_t entry_number = level_entry_number[level];

        if (current_table == NULL) {
            PANIC("null PageTable at level %d", level);
        }

        PRINTF_IF(DEBUG_MAP_ADDRESS,
                  "level %i = table @ 0x%x. using entry %d\n", level,
                  (uint64_t)current_table, entry_number);

        entry = &current_table[entry_number];

        // stop iterating at level 0
        if (level == 0) {
            break;
        }

        if (false == entry->flags.valid) {
            // if next level table has not been created
            // create it
            uint64_t new_page_addr = (uint64_t)alloc_page();
            uint64_t new_page_number = new_page_addr / PAGE_SIZE;

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

void print_PageTableEntry(PageTableEntry entry) {
    printf("Entry { flags: ");
    print_PageTableEntryFlags(entry.flags, true);
    printf(", physical_page_num: 0x%x }\n", entry.physical_page_num);
}

struct PrintPageTableRecurse {
    bool recurse;
    uint8_t level;
};
typedef struct PrintPageTableRecurse PrintPageTableRecurse;
#define PRINT_PAGE_TABLE_RECURSE_FROM_TOP \
    (PrintPageTableRecurse){.recurse = true, .level = 2}
#define PRINT_PAGE_TABLE_NO_RECURSE \
    (PrintPageTableRecurse){.recurse = false, .level = 0}

void print_PageTable(PageTable table, bool only_valid_entries,
                     PrintPageTableRecurse recurse) {
    for (size_t i = 0; i < (PAGE_SIZE / sizeof(uint64_t)); i++) {
        PageTableEntry entry = table[i];

        if (only_valid_entries && !entry.flags.valid) {
            continue;
        }

        if (recurse.recurse) {
            printf("(level[%d]) ", recurse.level);
        }

        printf("PageTable 0x%x entry[%d] = ", table, i);
        print_PageTableEntry(entry);

        if (recurse.recurse && !is_leaf_node(entry.flags)) {
            PrintPageTableRecurse next_recurse = recurse;
            next_recurse.level -= 1;
            print_PageTable(get_linked_table(entry), only_valid_entries,
                            next_recurse);
        }
    }
}

void activate_PageTable(PageTable table) {
    SatpRegister satp_register = {0};
    satp_register.mode = RISCV_MODE_SV39;
    satp_register.physical_page_num = (uint64_t)table / PAGE_SIZE;

    if (DEBUG_ACTIVATE_TABLE) {
        printf("activating page table (pid %d)\n", my_pid());
        print_PageTable(table, true, PRINT_PAGE_TABLE_RECURSE_FROM_TOP);
    }

    __asm__ __volatile__(
        "sfence.vma\n"
        "csrw satp, %[reg]\n"
        "sfence.vma\n" ::[reg] "r"(satp_register));
}

static const uint64_t memory_start = (uint64_t)__KERNEL_BASE;
static const uint64_t memory_end = (uint64_t)__PAGES_END;

void print_memory_size(void) {
    const uint64_t total_memory_space = memory_end - memory_start;
    const uint64_t total_pages = total_memory_space / PAGE_SIZE;

    printf("kernel memory space: %dkB = %d pages\n", total_memory_space / 1024,
           total_pages);
}

// enable virtual memory
void map_all_kernel_memory(PageTable table) {
    // TODO: mega/giga pages for different sections?

    // map entire kernel physical memory space
    for (uint64_t physical_address = memory_start;
         physical_address < memory_end; physical_address += PAGE_SIZE) {
        VirtualAddress virtual_address = {0};
        virtual_address.value = physical_address;

        PageTableEntryFlags flags = {0};
        // TODO: different flags for different sections
        flags.execute = true;
        flags.read = true;
        flags.write = true;

        map_address(table, virtual_address, physical_address, flags);
    }
}

uint64_t get_physical_address(PageTable table, VirtualAddress virtual_address) {
    uint64_t level_entry_number[] = {
        virtual_address.level0_entry_number,
        virtual_address.level1_entry_number,
        virtual_address.level2_entry_number,
    };
    PageTable current_table = table;
    PageTableEntry entry = {0};

    // TODO: refactor into macro/function? deduplicate map_address
    for (int level = 2; level >= 0; level--) {
        uint64_t entry_number = level_entry_number[level];

        if (current_table == NULL) {
            PANIC("null PageTable at level %d", level);
        }

        entry = current_table[entry_number];

        if (level == 0) {
            break;
        }

        if (false == entry.flags.valid) {
            PANIC("Invalid inner table at level %d", level);
        }

        current_table = get_linked_table(entry);
    }

    return ((entry.physical_page_num * PAGE_SIZE) +
            virtual_address.page_offset);
}
