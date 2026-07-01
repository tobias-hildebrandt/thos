#include "debug.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "paging.h"

// page alloc testing
void debug_page_alloc(void) {
    for (int page_num = 0; page_num < 8; page_num++) {
        char* page = (char*)alloc_page();

        // individually write each byte in page
        const size_t num_values = PAGE_SIZE / sizeof(char);
        for (size_t index = 0; index > num_values; index++) {
            page[index] = page_num;
        }

        printf("page alloc at 0x%x\n", (uint64_t)page);
    }
}

// trap test
void debug_trap(void) {
    // trigger trap
    __asm__ __volatile__("unimp");

    printf("this is a line after unimp\n");
}

// print testing
void debug_printf(void) {
    printf("printf hex 0          = 0x%x\n", 0x0);
    printf("printf hex 0xffffffff = 0x%x\n", 0xffffffff);
    printf("printf hex 0x01234567 = 0x%x\n", 0x01234567);
    printf("printf hex 0x89abcdef = 0x%x\n", 0x89abcdef);

    for (int i = -10; i <= 10; i++) {
        printf("%d ", i);
    }
    printf("\n");

    for (int i = -100000; i <= 100000; i += 10000) {
        printf("0x%x=%d\n", i, i);
    }
    printf("zero        0x%x=%d\n", 0, 0);
    printf("max int     0x%x=%d\n", INT32_MAX, INT32_MAX);
    printf("all bits 1  0x%x=%d\n", 0xffffffff, 0xffffffff);
    printf("min int     0x%x=%d\n", INT32_MIN, INT32_MIN);
}
