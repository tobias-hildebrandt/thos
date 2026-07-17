#pragma once

#include <stdio.h>   // IWYU pragma: keep, needed for printf
#include <stdlib.h>  // IWYU pragma: keep, needed for exit

#include "flags.h"  // IWYU pragma: keep, needed for PANIC_LOOP
#include "trap.h"   // IWYU pragma: keep, needed for disable_traps_now

// TODO: extract to function, simplify macros?

#if PANIC_LOOP == 0
#define PANIC(format_str, ...)                            \
    do {                                                  \
        disable_traps_now();                              \
        printf(                                           \
            "!!!\n"                                       \
            "Kernel panic!\n"                             \
            "%s:%u (%s)\n"                                \
            "Reason: " format_str "\n!!!\n",              \
            __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        exit(1);                                          \
    } while (0)
#else
#define PANIC(format_str, ...)                            \
    do {                                                  \
        disable_traps_now();                              \
        printf(                                           \
            "!!!\n"                                       \
            "Kernel panic!\n"                             \
            "%s:%u %s()\n"                                \
            "Reason: " format_str "\n!!!\n",              \
            __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        while (1) {                                       \
        };                                                \
    } while (0)
#endif
