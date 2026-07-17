#pragma once

#define BIT_SET(value, bit_number) (value) |= (1 << (bit_number))
#define BIT_UNSET(value, bit_number) (value) &= ~(1 << (bit_number))
#define BIT_BOOL(value, bit_number, bool) \
    do {                                  \
        if (bool) {                       \
            BIT_SET(value, bit_number);   \
        } else {                          \
            BIT_UNSET(value, bit_number); \
        }                                 \
    } while (0)

#define BIT_GET(value, bit_number) ((bool)((value) & (1 << (bit_number))))
#define BIT_TO_INT(bit_number) (1 << (bit_number))
