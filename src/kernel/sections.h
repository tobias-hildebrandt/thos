#pragma once

#include <stddef.h>  // IWYU pragma: keep
#include <stdint.h>

#include "paging.h"  // IWYU pragma: keep
#include "util.h"    // IWYU pragma: keep

// declares <section>_START and <section>_END as extern uint64_t's
#define SECTION_DECLARE(section)                   \
    extern const uint64_t CONCAT_(section, START); \
    extern const uint64_t CONCAT_(section, END)

// returns size of section
#define SECTION_SIZE(section) (CONCAT_(section, END) - CONCAT_(section, START))

SECTION_DECLARE(MEMORY);
SECTION_DECLARE(TEXT);
SECTION_DECLARE(RODATA);
SECTION_DECLARE(BSS);
// don't forget stack is "backwards!"
SECTION_DECLARE(STACK);
SECTION_DECLARE(PAGES);

SECTION_DECLARE(USER_user);
SECTION_DECLARE(USER_user2);

void print_all_sections(void);
