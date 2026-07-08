#pragma once

#include <stdio.h>   // IWYU pragma: keep, printf
#include <stdlib.h>  // IWYU pragma: keep, exit

#define static_assert _Static_assert

#ifndef NDEBUG
#ifndef ASSERT_EXIT
#define ASSERT_EXIT 1
#endif
#define assert(condition)                                                  \
    do {                                                                   \
        if (!(condition)) {                                                \
            printf("assertion failed: %s:%u (%s): assert(%s)\n", __FILE__, \
                   __LINE__, __func__, #condition);                        \
            if (ASSERT_EXIT) {                                             \
                exit(1);                                                   \
            }                                                              \
        }                                                                  \
    } while (0)
#else
#define assert(condition) ((void)0)
#endif
