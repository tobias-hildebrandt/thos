#pragma once

#include "io.h"   // IWYU pragma: keep, needed for printf
#include "sbi.h"  // IWYU pragma: keep, needed for sbi_shutdown

#define PANIC(format_str, ...)                                             \
    do {                                                                   \
        printf("!!!\nKernel panic!\n%s:%d\nReason: " format_str "\n!!!\n", \
               __FILE__, __LINE__, ##__VA_ARGS__);                         \
        sbi_shutdown();                                                    \
    } while (0)
