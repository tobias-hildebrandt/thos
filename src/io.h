#pragma once

void put_char(const char ch);

void printf(const char* format_str, ...);

#define PRINTF_IF(def, ...)      \
    do {                         \
        if (def) {               \
            printf(__VA_ARGS__); \
        }                        \
    } while (0)
