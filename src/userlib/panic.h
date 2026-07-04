#pragma once

// TODO: make exit program

#define PANIC(format_str, ...)                            \
    do {                                                  \
        printf(                                           \
            "User panic\n"                                \
            "%s:%u %s()\n"                                \
            "Reason: " format_str "\n!!!\n",              \
            __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
        while (1) {                                       \
        };                                                \
    } while (0)
