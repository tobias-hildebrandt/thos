#pragma once

#include "io.h"   // IWYU pragma: keep, needed for printf
#include "sbi.h"  // IWYU pragma: keep, needed for sbi_shutdown

#define PANIC(format_str, ...)                            \
    do {                                                  \
        printf(                                           \
            "!!!\n"                                       \
            "Kernel panic!\n"                             \
            "%s:%d (in func %s)\n"                        \
            "Reason: " format_str "\n!!!\n",              \
            __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        sbi_shutdown();                                   \
    } while (0)
