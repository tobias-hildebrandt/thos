#pragma once

#include <stdio.h>  // IWYU pragma: keep, needed for printf

#define PRINTF_IF(def, ...)      \
    do {                         \
        if (def) {               \
            printf(__VA_ARGS__); \
        }                        \
    } while (0)
