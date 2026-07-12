#pragma once

#include <stdint.h>

#include "paging.h"  // IWYU pragma: keep
#include "util.h"    // IWYU pragma: keep

// declares <section>_START and <section>_END as extern uintptr_t's
#define SECTION_DECLARE(section)                    \
    extern const uintptr_t CONCAT_(section, START); \
    extern const uintptr_t CONCAT_(section, END)

// returns size of section
#define SECTION_SIZE(section) (CONCAT_(section, END) - CONCAT_(section, START))

SECTION_DECLARE(MEMORY);
SECTION_DECLARE(TEXT);
SECTION_DECLARE(RODATA);
SECTION_DECLARE(BSS);
// don't forget stack is "backwards!"
SECTION_DECLARE(STACK);
SECTION_DECLARE(PAGES);

// TODO: pass in via commandline -D define?
SECTION_DECLARE(USER_first);
SECTION_DECLARE(USER_second);

// places symbol in the global special page
#define IN_GLOBAL_SPECIAL __attribute__((section(".global_special")))
// global special page physical address
extern const uintptr_t GLOBAL_SPECIAL_PAGE;

#define IN_USER_SPECIAL __attribute__((section(".user_special")))
extern const uintptr_t USER_SPECIAL_PAGE;

void print_all_sections(void);
