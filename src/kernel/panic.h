#pragma once

#include <stdio.h>  // IWYU pragma: keep, needed for printf

#include "flags.h"  // IWYU pragma: keep, needed for PANIC_LOOP
#include "sbi.h"    // IWYU pragma: keep, needed for sbi_shutdown

// TODO: extract to function, simplify macros?

#if PANIC_LOOP == 0
#define PANIC(format_str, ...)                            \
    do {                                                  \
        printf(                                           \
            "!!!\n"                                       \
            "Kernel panic!\n"                             \
            "%s:%d %s()\n"                                \
            "Reason: " format_str "\n!!!\n",              \
            __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        sbi_shutdown();                                   \
    } while (0)
#else
#define PANIC(format_str, ...)                            \
    do {                                                  \
        printf(                                           \
            "!!!\n"                                       \
            "Kernel panic!\n"                             \
            "%s:%d %s()\n"                                \
            "Reason: " format_str "\n!!!\n",              \
            __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        while (1) {                                       \
        };                                                \
    } while (0)
#endif
