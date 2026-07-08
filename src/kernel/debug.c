#include "debug.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "build_info.h"
#include "paging.h"
#include "util.h"

// page alloc testing
void debug_page_alloc(void) {
    for (int page_num = 0; page_num < 8; page_num++) {
        char* page = (char*)alloc_page();

        // individually write each byte in page
        const size_t num_values = PAGE_SIZE / sizeof(char);
        for (size_t index = 0; index > num_values; index++) {
            page[index] = page_num;
        }

        printf("page alloc at %p\n", page);
    }
}

// print testing
void debug_printf(void) {
    printf("printf int hex 0          = 0x%x\n", 0x0);
    printf("printf int hex 0xffffffff = 0x%x\n", 0xffffffff);
    printf("printf int hex 0x01234567 = 0x%x\n", 0x01234567);
    printf("printf int hex 0x89abcdef = 0x%x\n", 0x89abcdef);

    printf("printf longlong hex 0                  = 0x%llx\n", 0x0LL);
    printf("printf longlong hex 0xffffffffffffffff = 0x%llx\n",
           0xffffffffffffffffLL);
    printf("printf longlong hex 0x0123456789abcdef = 0x%llx\n",
           0x0123456789abcdefLL);

    printf("printf %%x but pass longlong: 0x0123456789abcdef = 0x%x\n",
           0x0123456789abcdefLL);
    printf("printf %%llx but pass int: 0x01234567 = 0x%llx\n",
           ((int)0x01234567));

    printf("printf alt# int hex 0          = %#x\n", 0x0);
    printf("printf alt# int hex 0xffffffff = %#x\n", 0xffffffff);
    printf("printf alt# int hex 0x01234567 = %#x\n", 0x01234567);
    printf("printf alt# int hex 0x89abcdef = %#x\n", 0x89abcdef);

    printf("printf alt# longlong hex 0                  = %#llx\n", 0x0LL);
    printf("printf alt# longlong hex 0xffffffffffffffff = %#llx\n",
           0xffffffffffffffffLL);
    printf("printf alt# longlong hex 0x0123456789abcdef = %#llx\n",
           0x0123456789abcdefLL);

    for (int i = -10; i <= 10; i++) {
        printf("%d ", i);
    }
    printf("\n");

    for (int i = -100000; i <= 100000; i += 10000) {
        printf("0x%x = %d\n", i, i);
    }

    for (long shift = 0; shift < POINTER_BITS; shift += 4) {
        uintptr_t value = (1L) << shift;
        printf("0x%p = signed=%p unsigned=%p\n", value, value, value);
    }

    printf("zero int        0x%x = %d\n", 0, 0);
    printf("max int         0x%x = %d\n", INT32_MAX, INT32_MAX);
    printf("all 1s int      0x%x = %d\n", 0xffffffff, 0xffffffff);
    printf("min int         0x%x = %d\n", INT32_MIN, INT32_MIN);
    printf("zero uint       0x%x = %u\n", 0, 0);
    printf("max uint        0x%x = %u\n", UINT32_MAX, UINT32_MAX);

    printf("zero longlong   0x%lx = %lld\n", 0, 0);
    printf("max longlong    0x%lx = %lld\n", INT64_MAX, INT64_MAX);
    printf("all 1s longlong 0x%lx = %lld\n", 0xffffffffffffffff,
           0xffffffffffffffff);
    printf("min longlong    0x%lx = %lld\n", INT64_MIN, INT64_MIN);
    printf("zero ulonglong  0x%lx = %llu\n", 0, 0);
    printf("max ulonglong   0x%lx = %llu\n", UINT64_MAX, UINT64_MAX);

    int printed = printf("0123456789");
    printf("\n");

    printf("^printed %d characters (not including newline)\n", printed);

    assert(10 == printed);

    printf("min width 10 \"abc\":               _%10s_\n", "abc");
    printf("min width 10 \"12345\":             _%10u_\n", 12345);
    printf("min width 10 \"-12345\":            _%10d_\n", -12345);
    printf("min width 10 \"ffff\":              _%10x_\n", 0xffff);

    printf("min width 10, zero-pad \"12345\":   _%010u_\n", 12345);
    printf("min width 10, zero-pad \"ffff\":    _%010x_\n", 0xffff);
    printf("min width 8, zero-pad \"ffff\":     _%08x_\n", 0xffff);

    printf("pointer 0x0:                %p\n", 0);
    printf("pointer 0xffff:             %p\n", 0xffff);
    ONLY64(printf("pointer 0x0123456789abcdef: %p\n", 0x0123456789abcdefLL));
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

    PRINT_ATOX(atoll, "%lld", "0", 0LL);
    PRINT_ATOX(atoll, "%lld", "-1000000000000000", -1000000000000000LL);
    PRINT_ATOX(atoll, "%lld", "1000000000000000", 1000000000000000LL);

    PRINT_ATOX(atoi, "%d", "", 0);
    PRINT_ATOX(atoi, "%d", "aaaaaaa", 0);
    PRINT_ATOX(atoi, "%d", "+aaaaa", 0);
    PRINT_ATOX(atoi, "%d", "++++111", 0);
    PRINT_ATOX(atoi, "%d", "    \t\n\t\n       ", 0);
}

#define PRINT_IS_ALIGNED(val, align, expected)                        \
    printf("is_aligned(%d, %d) = %s, should be %s, %s\n", val, align, \
           is_aligned(val, align) ? "true" : "false",                 \
           expected ? "true" : "false",                               \
           is_aligned(val, align) == expected ? "OK" : "FAIL")
#define PRINT_ALIGN_UP(val, align, expected)                        \
    printf("align_up(%d, %d) = %d, should be %d, %s\n", val, align, \
           align_up(val, align), expected,                          \
           align_up(val, align) == expected ? "OK" : "FAIL")

// alignment testing
void debug_align(void) {
    PRINT_IS_ALIGNED(8, 4, true);
    PRINT_IS_ALIGNED(9, 4, false);
    PRINT_IS_ALIGNED(4096, 16, true);

    PRINT_ALIGN_UP(5, 8, 8);
    PRINT_ALIGN_UP(8, 8, 8);
    PRINT_ALIGN_UP(13, 8, 16);
    PRINT_ALIGN_UP(13, 2, 14);
}
