#include "paging.h"

#include <stdbool.h>
#include <string.h>

#include "flags.h"
#include "io.h"
#include "panic.h"
#include "sections.h"
#include "util.h"

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

static uint64_t next_page = 0;

uint64_t alloc_page(void) {
    if (next_page == 0) {
        next_page = (uint64_t)__PAGES_START;
    }

    if (next_page > (uint64_t)__PAGES_END - PAGE_SIZE) {
        PANIC("page allocation would overflow page memory!");
    }

    uint64_t this_page = next_page;
    next_page += PAGE_SIZE;

    // wipe page
    memset((void*)this_page, 0, PAGE_SIZE);

    return this_page;
}

PageTable get_linked_table(PageTableEntry* entry) {
    if (entry == NULL) {
        PANIC("passed null entry to get_linked_table");
    }
    if (false == entry->flags.valid) {
        PANIC("passed invalid entry to get_linked_table");
    }
    uint64_t page_num = entry->physical_page_num;
    uint64_t next_table_addr = page_num * PAGE_SIZE;

    return (PageTable)next_table_addr;
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
    PRINTF_IF(
        DEBUG_MAP_ADDRESS, "map_address(table @ 0x%x, v:0x%x, p:0x%x, f:%x)\n",
        (uint64_t)first_table, virtual_address.value, physical_address, flags);

    if (!is_aligned(virtual_address.value, PAGE_SIZE)) {
        PANIC("virtual address not aligned %x", virtual_address.value);
    }

    if (!is_aligned(physical_address, PAGE_SIZE)) {
        PANIC("physical address not aligned %x", physical_address);
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
    for (int i = 2; i >= 0; i--) {
        uint64_t entry_number = level_entry_number[i];

        if (current_table == NULL) {
            PANIC("null PageTable at level %d", i);
        }

        PRINTF_IF(DEBUG_MAP_ADDRESS, "level %i = table @ 0x%x. entry #%d\n", i,
                  (uint64_t)current_table, entry_number);

        entry = &current_table[entry_number];

        // stop iterating at level 0
        if (i == 0) {
            break;
        }

        if (false == entry->flags.valid) {
            // if next level table has not been created
            // create it
            uint64_t new_page_addr = alloc_page();
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
            current_table = get_linked_table(entry);
        }
    }

    // set data in level 0 entry
    entry->physical_page_num = physical_address / PAGE_SIZE;
    entry->flags.value = flags.value;

    // make sure it's valid
    entry->flags.valid = true;
}

void print_PageTableEntryFlags(PageTableEntryFlags flags) {
    printf("PTEFlags{v:%d,r:%d,w:%d,x:%d,u:%d,g:%d,a:%d,d:%d}", flags.valid,
           flags.read, flags.write, flags.execute, flags.user, flags.global,
           flags.accessed, flags.dirty);
}

void print_PageTableEntry(PageTableEntry* entry) {
    printf("PageTableEntry {\n\t");
    print_PageTableEntryFlags(entry->flags);
    printf(",\n\tphysical_page_num: %x\n}\n", entry->physical_page_num);
}

void print_PageTable(PageTable table, bool only_valid_entries) {
    for (size_t i = 0; i < (PAGE_SIZE / sizeof(uint64_t)); i++) {
        PageTableEntry* entry = &table[i];

        if (only_valid_entries && !entry->flags.valid) {
            continue;
        }
        printf("PageTable[%d] = ", i);
        print_PageTableEntry(entry);
    }
}

void set_active_PageTable(PageTable table) {
    SatpRegister satp_register = {0};
    satp_register.mode = RISCV_MODE_SV39;
    satp_register.physical_page_num = (uint64_t)table / PAGE_SIZE;

    __asm__ __volatile__(
        "sfence.vma\n"
        "csrw satp, %[reg]\n"
        "sfence.vma\n" ::[reg] "r"(satp_register));
}

// enable virtual memory
void enable_virtual_memory(void) {
    const uint64_t memory_start = (uint64_t)__KERNEL_BASE;
    const uint64_t memory_end = (uint64_t)__PAGES_END;
    const uint64_t total_memory_space = memory_end - memory_start;
    const uint64_t total_pages = total_memory_space / PAGE_SIZE;

    printf("kernel memory space: %dB = %d pages\n", total_memory_space,
           total_pages);

    PageTable kernel_table = (PageTable)alloc_page();

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

        map_address(kernel_table, virtual_address, physical_address, flags);
    }

    set_active_PageTable(kernel_table);
}
