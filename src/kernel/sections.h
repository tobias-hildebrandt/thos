#pragma once

#include <stdint.h>

#include "util.h"  // IWYU pragma: keep, SECTION

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

// special sections
SECTION_DECLARE(GLOBAL_SPECIAL);
SECTION_DECLARE(USER_SPECIAL);

// places symbol in the global special page
#define IN_GLOBAL_SPECIAL SECTION(".global_special")
// places symbol in the user special page
#define IN_USER_SPECIAL SECTION(".user_special")

void print_all_sections(void);
