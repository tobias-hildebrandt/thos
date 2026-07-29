#include "sections.h"

#include <stdint.h>
#include <stdio.h>

#include "util.h"
#include "virtual_memory/page.h"

SECTION_IMPL(MEMORY, SECTION_TYPE_META);
SECTION_IMPL(USER_PROGRAMS, SECTION_TYPE_META);

SECTION_IMPL(TEXT, SECTION_TYPE_NORMAL);
SECTION_IMPL(RODATA, SECTION_TYPE_NORMAL);
SECTION_IMPL(BSS, SECTION_TYPE_NORMAL);
SECTION_IMPL(PAGES, SECTION_TYPE_NORMAL);

SECTION_IMPL(GLOBAL_SPECIAL, SECTION_TYPE_SPECIAL);
SECTION_IMPL(USER_SPECIAL, SECTION_TYPE_SPECIAL);

static char* SectionType_str(SectionType type) {
    switch (type) {
        case SECTION_TYPE_NORMAL:
            return "normal";
        case SECTION_TYPE_META:
            return "meta";
        case SECTION_TYPE_SPECIAL:
            return "special";
        case SECTION_TYPE_USER_PROGRAM:
            return "user program";
        default:
            return "invalid section type";
    }
}

uintptr_t Section_size(const Section* section) {
    return section->end_address - section->start_address;
}

void Section_print(const Section* section) {
    uintptr_t size = Section_size(section);
    uintptr_t kibibytes = INT_DIV_CEIL(size, 1024);
    uintptr_t pages = INT_DIV_CEIL(size, PAGE_SIZE);
    printf("section %s\n", section->name);
    printf("\ttype:       %s\n", SectionType_str(section->type));
    printf("\taddresses:  %p - %p\n", section->start_address,
           section->end_address);
    printf("\tsize:       %lu pages (~%lu kB, %lu bytes)\n", pages, kibibytes,
           size);
}

#define PRINT_SECTION(section) Section_print(&CONCAT_(SECTION, section))

void print_all_sections(void) {
    PRINT_SECTION(MEMORY);

    PRINT_SECTION(TEXT);
    PRINT_SECTION(RODATA);
    PRINT_SECTION(BSS);
    PRINT_SECTION(PAGES);

    PRINT_SECTION(GLOBAL_SPECIAL);
    PRINT_SECTION(USER_SPECIAL);

    PRINT_SECTION(USER_PROGRAMS);

    for (size_t i = 0; i < num_user_programs; i++) {
        Section_print(&user_programs[i]);
    }
}
