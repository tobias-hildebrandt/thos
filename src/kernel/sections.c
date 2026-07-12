#include "sections.h"

#include <stdint.h>
#include <stdio.h>

#include "paging.h"
#include "util.h"

// grabs the actual __<section>_START and __<section>_END defs, which are set in
// the linker script
// assigns values to symbols declared in header
#define SECTION_IMPL(section)                        \
    extern const char CONCAT_(__##section, START)[]; \
    extern const char CONCAT_(__##section, END)[];   \
    const uintptr_t CONCAT_(section, START) =        \
        (uintptr_t)CONCAT_(__##section, START);      \
    const uintptr_t CONCAT_(section, END) = (uintptr_t)CONCAT_(__##section, END)

SECTION_IMPL(MEMORY);
SECTION_IMPL(TEXT);
SECTION_IMPL(RODATA);
SECTION_IMPL(BSS);
SECTION_IMPL(STACK);
SECTION_IMPL(PAGES);

SECTION_IMPL(USER_first);
SECTION_IMPL(USER_second);

extern const char __GLOBAL_SPECIAL_PAGE[];
const uintptr_t GLOBAL_SPECIAL_PAGE = (uintptr_t)__GLOBAL_SPECIAL_PAGE;
extern const char __USER_SPECIAL_PAGE[];
const uintptr_t USER_SPECIAL_PAGE = (uintptr_t)__USER_SPECIAL_PAGE;

#define PRINT_SECTION(section)                                              \
    print_section(#section, CONCAT_(section, START), CONCAT_(section, END), \
                  SECTION_SIZE(section))

void print_section(char* name, uintptr_t start, uintptr_t end, uintptr_t size) {
    uintptr_t kb = INT_DIV_CEIL(size, 1024);
    uintptr_t pages = INT_DIV_CEIL(size, PAGE_SIZE);
    printf("section %s\n", name);
    printf("\taddresses:  %p - %p\n", start, end);
    printf("\tsize:       %lu pages (~%lu kB, %lu bytes)\n", pages, kb, size);
}

void print_all_sections(void) {
    PRINT_SECTION(MEMORY);
    PRINT_SECTION(TEXT);
    PRINT_SECTION(RODATA);
    PRINT_SECTION(BSS);
    PRINT_SECTION(STACK);
    PRINT_SECTION(PAGES);

    PRINT_SECTION(USER_first);
    PRINT_SECTION(USER_second);
}
