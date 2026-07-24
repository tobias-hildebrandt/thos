#include "virtual_memory/page.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "panic.h"
#include "sections.h"

static void* next_page = NULL;

Page Page_alloc(void) {
    if (next_page == 0) {
        // NOLINTNEXTLINE(performance-no-int-to-ptr)
        next_page = (void*)PAGES_START;
    }

    if ((uintptr_t)next_page > (PAGES_END - PAGE_SIZE)) {
        PANIC("page allocation would overflow page memory!");
    }

    void* this_page = next_page;
    next_page = (char*)next_page + PAGE_SIZE;

    // wipe page
    memset(this_page, 0, PAGE_SIZE);

    return this_page;
}
