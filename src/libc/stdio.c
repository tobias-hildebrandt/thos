#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "panic.h"

// https://en.cppreference.com/c/io/fprintf
// %[modifiers][min width][.precision][type length specifier]conversion_format

struct PrintState {
    bool in_conversion_spec;
    bool long_modifier;
    bool zero_pad;
    int min_width;
};
typedef struct PrintState PrintState;

void PrintState_reset(PrintState* state) {
    *state = (PrintState){0};
}

bool PrintState_is_clean(PrintState* state) {
    return state->long_modifier == 0 && state->long_modifier == 0 &&
           state->min_width == 0;
}

void print_string(char* str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        putchar(str[i]);
    }
}

void print_hex(uint64_t value, PrintState* state) {
    int i;
    if (state->long_modifier) {
        i = 16;
    } else {
        i = 8;
    }

    for (; i > 0; i--) {
        unsigned int shift_amount = i - 1;
        unsigned int hex = (value >> (shift_amount * 4)) & 0xf;
        unsigned int ascii;
        if (hex <= 9) {
            ascii = hex + '0';
        } else {
            ascii = hex + 'a' - 10;
        }
        putchar(ascii);
    }
}

void print_signed(int64_t signed_value, PrintState* state) {
    // highest possible number of decimal digits =
    // ceil(log10(2^(bitlen-1)))
    // = 10 (32bit) or 19 (64bit)
    uint64_t divisor;
    if (state->long_modifier) {
        divisor = 1000000000000000000;
    } else {
        const uint32_t int_divisor = 1000000000;
        divisor = (uint64_t)int_divisor;
    }

    if (signed_value < 0) {
        putchar('-');
        // TODO: just OR all bits except MSB?
        signed_value *= -1;
    }
    uint64_t positive_value = (uint64_t)signed_value;
    bool started = false;

    // edge case that's easier to handle here
    if (positive_value == 0) {
        putchar('0');
        return;
    }

    while (divisor > 0) {
        if (positive_value >= divisor) {
            started = true;
            const uint64_t digit = positive_value / divisor;
            positive_value -= digit * divisor;
            putchar('0' + digit);
        } else if (started) {
            putchar('0');
        } else {
            // do nothing!
        }

        divisor /= 10;
    }
}

void printf(const char* format_str, ...) {
    va_list args;
    va_start(args, format_str);

    PrintState state = {0};
    while (*format_str != '\0') {
        if (!state.in_conversion_spec) {
            // if we aren't currently parsing a conversion spec
            if (*format_str == '%') {
                state.in_conversion_spec = true;
            } else {
                putchar(*format_str);
            }
        } else {
            // we are currently parsing a conversion spec
            switch (*format_str) {
                case '\0': {
                    // undefined behavior
                    PANIC("end of format string inside of conversion spec");
                }
                // modifiers
                case 'l': {
                    // long
                    state.long_modifier = true;
                    break;
                }
                // conversion specifiers
                case '%': {
                    // %%
                    if (!PrintState_is_clean(&state)) {
                        PANIC(
                            "printf conversion specifier was %%, but had "
                            "modifiers");
                    }
                    putchar('%');
                    PrintState_reset(&state);
                    break;
                }
                case 'c': {
                    // character
                    int c = va_arg(args, int);
                    putchar(c);
                    PrintState_reset(&state);
                    break;
                }
                case 's': {
                    // string
                    char* str = va_arg(args, char*);
                    print_string(str);
                    PrintState_reset(&state);
                    break;
                }
                case 'x': {
                    // hexadecimal
                    uint64_t value = va_arg(args, uint64_t);
                    print_hex(value, &state);
                    PrintState_reset(&state);
                    break;
                }
                case 'i':
                case 'd': {
                    // signed decimal integer
                    int64_t signed_value = va_arg(args, int64_t);
                    print_signed(signed_value, &state);
                    PrintState_reset(&state);
                    break;
                }
                default: {
                    PANIC("unknown conversion specification character: %c",
                          *format_str);
                }
            }
        }

        // go to next character
        format_str += 1;
    }

    va_end(args);
}
