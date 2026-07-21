#pragma once

#include <stdint.h>
#include <stdio.h>  // IWYU pragma: keep, needed for printf

#include "buffer.h"

#define PRINTF_IF(def, ...)      \
    do {                         \
        if (def) {               \
            printf(__VA_ARGS__); \
        }                        \
    } while (0)

int putchar_buffer(int character, Buffer* buffer);

// print values directly via sbi_putchar, skipping locking mechanisms
// TODO: use s/f printf
void put_direct_str(const char* str);
void put_direct_hex32(uint32_t val);
void put_direct_u32(uint32_t val);
