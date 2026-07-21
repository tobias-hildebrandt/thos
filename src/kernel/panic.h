#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>  // IWYU pragma: keep, printf

#include "flags.h"
#include "util.h"

// TODO: extract to function, simplify macros?

// TODO: panic-safe printf (via some variant of printf: s/sn/f?)

#if PANIC_SAFE_PRINT
#define PANIC(...)                                 \
    do {                                           \
        _panic_unused(0, __VA_ARGS__);             \
        _panic_safe(__FILE__, __LINE__, __func__); \
    } while (0)
#else
#define PANIC(format_str, ...)                            \
    do {                                                  \
        _panic_start();                                   \
        printf(                                           \
            "!!!\n"                                       \
            "Kernel panic!\n"                             \
            "%s:%u (%s)\n"                                \
            "Reason: " format_str "\n!!!\n",              \
            __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        _panic_end();                                     \
    } while (0)
#endif

void _panic_start(void);
NORETURN void _panic_end(void);
void _panic_unused(UNUSED int unused, ...);
NORETURN void _panic_safe(const char* file, uint64_t line, const char* func);
