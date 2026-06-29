#include "alloc.h"

#include "mem.h"  // IWYU pragma: keep
#include "panic.h"
#include "types.h"  // IWYU pragma: keep

extern char __PAGES_START[], __PAGES_END[];

static uint64_t next_page = 0;

uint64_t alloc_page() {
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
