#include "debug.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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

        printf("page alloc at 0x%lx\n", (uint64_t)page);
    }
}

// print testing
void debug_printf(void) {
    printf("printf int hex 0          = 0x%x\n", 0x0);
    printf("printf int hex 0xffffffff = 0x%x\n", 0xffffffff);
    printf("printf int hex 0x01234567 = 0x%x\n", 0x01234567);
    printf("printf int hex 0x89abcdef = 0x%x\n", 0x89abcdef);

    printf("printf long hex 0                  = 0x%lx\n", 0x0);
    printf("printf long hex 0xffffffffffffffff = 0x%lx\n", 0xffffffffffffffff);
    printf("printf long hex 0x0123456789abcdef = 0x%lx\n", 0x0123456789abcdef);

    printf("printf %%x but pass long: 0x0123456789abcdef = 0x%x\n",
           0x0123456789abcdef);
    printf("printf %%lx but pass int: 0x01234567 = 0x%lx\n",
           ((uint32_t)0x01234567));

    printf("printf alt# int hex 0          = %#x\n", 0x0);
    printf("printf alt# int hex 0xffffffff = %#x\n", 0xffffffff);
    printf("printf alt# int hex 0x01234567 = %#x\n", 0x01234567);
    printf("printf alt# int hex 0x89abcdef = %#x\n", 0x89abcdef);

    printf("printf alt# long hex 0                  = %#lx\n", 0x0);
    printf("printf alt# long hex 0xffffffffffffffff = %#lx\n",
           0xffffffffffffffff);
    printf("printf alt# long hex 0x0123456789abcdef = %#lx\n",
           0x0123456789abcdef);

    for (int i = -10; i <= 10; i++) {
        printf("%d ", i);
    }
    printf("\n");

    for (int i = -100000; i <= 100000; i += 10000) {
        printf("0x%x = %d\n", i, i);
    }

    for (long shift = 0; shift < 64; shift += 4) {
        long value = (1L) << shift;
        printf("0x%lx = signed=%ld unsigned=%lu\n", value, value, value);
    }

    printf("zero int        0x%x = %d\n", 0, 0);
    printf("max int         0x%x = %d\n", INT32_MAX, INT32_MAX);
    printf("all 1s int      0x%x = %d\n", 0xffffffff, 0xffffffff);
    printf("min int         0x%x = %d\n", INT32_MIN, INT32_MIN);
    printf("zero uint       0x%x = %u\n", 0, 0);
    printf("max uint        0x%x = %u\n", UINT32_MAX, UINT32_MAX);

    printf("zero long       0x%lx = %ld\n", 0, 0);
    printf("max long        0x%lx = %ld\n", INT64_MAX, INT64_MAX);
    printf("all 1s long     0x%lx = %ld\n", 0xffffffffffffffff,
           0xffffffffffffffff);
    printf("min long        0x%lx = %ld\n", INT64_MIN, INT64_MIN);
    printf("zero ulong      0x%lx = %lu\n", 0, 0);
    printf("max ulong       0x%lx = %lu\n", UINT64_MAX, UINT64_MAX);

    int printed = printf("0123456789");

    printf("^printed %d characters (not including newline)\n", printed);

    printf("min width 10 \"abc\":               _%10s_\n", "abc");
    printf("min width 10 \"12345\":             _%10u_\n", 12345);
    printf("min width 10 \"-12345\":            _%10d_\n", -12345);
    printf("min width 10 \"ffff\":              _%10x_\n", 0xffff);

    printf("min width 10, zero-pad \"12345\":   _%010u_\n", 12345);
    printf("min width 10, zero-pad \"ffff\":    _%010x_\n", 0xffff);
    printf("min width 8, zero-pad \"ffff\":     _%08x_\n", 0xffff);

    printf("pointer 0x0:                %p\n", 0);
    printf("pointer 0xffff:             %p\n", 0xffff);
    printf("pointer 0x0123456789abcdef: %p\n", 0x0123456789abcdef);
}

// TODO: assert
#define PRINT_ATOX(func, printfmt, number, expected)                           \
    printf(#func "(" #number ") = " printfmt ", should be " printfmt ", %s\n", \
           func(number), expected, func(number) == expected ? "OK" : "FAIL!")

// integer parsing testing
void debug_atoi(void) {
    PRINT_ATOX(atoi, "%d", "0", 0);
    PRINT_ATOX(atoi, "%d", "10", 10);
    PRINT_ATOX(atoi, "%d", "999999", 999999);
    PRINT_ATOX(atoi, "%d", "0000999999", 999999);

    PRINT_ATOX(atoi, "%d", "+10", 10);
    PRINT_ATOX(atoi, "%d", "+999999", 999999);
    PRINT_ATOX(atoi, "%d", "-10", -10);
    PRINT_ATOX(atoi, "%d", "-999999", -999999);

    PRINT_ATOX(atoi, "%d", "     10", 10);
    PRINT_ATOX(atoi, "%d", "     10        ", 10);
    PRINT_ATOX(atoi, "%d", "-999999asdfeswesgrarewf", -999999);
    PRINT_ATOX(atoi, "%d", "0x123", 0);  // prank'd!

    PRINT_ATOX(atol, "%ld", "0", 0);
    PRINT_ATOX(atol, "%ld", "-1000000000000000", -1000000000000000);
    PRINT_ATOX(atol, "%ld", "1000000000000000", 1000000000000000);

    PRINT_ATOX(atoi, "%d", "", 0);
    PRINT_ATOX(atoi, "%d", "aaaaaaa", 0);
    PRINT_ATOX(atoi, "%d", "+aaaaa", 0);
    PRINT_ATOX(atoi, "%d", "++++111", 0);
    PRINT_ATOX(atoi, "%d", "    \t\n\t\n       ", 0);
}
