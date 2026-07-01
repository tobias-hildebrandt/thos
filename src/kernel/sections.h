#pragma once

#include <stddef.h>  // IWYU pragma: keep
#include <stdint.h>

#include "paging.h"  // IWYU pragma: keep
#include "util.h"    // IWYU pragma: keep

#define SECTION_CONCAT(a, b) a##_##b

// declares <section>_START and <section>_END as extern uint64_t's
#define SECTION_DECLARE(section)                    \
    extern uint64_t SECTION_CONCAT(section, START); \
    extern uint64_t SECTION_CONCAT(section, END)

// returns size of section
#define SECTION_SIZE(section) \
    (SECTION_CONCAT(section, END) - SECTION_CONCAT(section, START))

SECTION_DECLARE(MEMORY);
SECTION_DECLARE(TEXT);
SECTION_DECLARE(RODATA);
SECTION_DECLARE(BSS);
// don't forget stack is "backwards!"
SECTION_DECLARE(STACK);
SECTION_DECLARE(PAGES);

void print_all_sections(void);
