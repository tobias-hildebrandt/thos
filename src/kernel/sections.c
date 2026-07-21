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

SECTION_IMPL(GLOBAL_SPECIAL);
SECTION_IMPL(USER_SPECIAL);

#define PRINT_SECTION(section)                                              \
    print_section(#section, CONCAT_(section, START), CONCAT_(section, END), \
                  SECTION_SIZE(section))

static void print_section(char* name, uintptr_t start, uintptr_t end,
                          uintptr_t size) {
    uintptr_t kibibytes = INT_DIV_CEIL(size, 1024);
    uintptr_t pages = INT_DIV_CEIL(size, PAGE_SIZE);
    printf("section %s\n", name);
    printf("\taddresses:  %p - %p\n", start, end);
    printf("\tsize:       %lu pages (~%lu kB, %lu bytes)\n", pages, kibibytes,
           size);
}

void print_all_sections(void) {
    PRINT_SECTION(MEMORY);
    PRINT_SECTION(TEXT);
    PRINT_SECTION(RODATA);
    PRINT_SECTION(BSS);
    PRINT_SECTION(STACK);
    PRINT_SECTION(PAGES);

    PRINT_SECTION(GLOBAL_SPECIAL);
    PRINT_SECTION(USER_SPECIAL);

    PRINT_SECTION(USER_first);
    PRINT_SECTION(USER_second);
}
