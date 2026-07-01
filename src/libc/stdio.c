#include <stdarg.h>
#include <stdbool.h>
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

int print_string(char* str) {
    int i = 0;
    for (; str[i] != '\0'; i++) {
        putchar(str[i]);
    }

    return i;
}

int print_hex(uint64_t value, PrintState* state) {
    int printed = 0;
    int shift;
    if (state->long_modifier) {
        shift = 16 - 1;
    } else {
        shift = 8 - 1;
    }

    for (; shift >= 0; shift--) {
        unsigned int place_value = (value >> (shift * 4)) & 0xf;
        unsigned int ascii;
        if (place_value <= 9) {
            ascii = '0' + place_value;
        } else {
            ascii = 'a' + place_value - 10;
        }
        putchar(ascii);
        printed += 1;
    }

    return printed;
}

int print_unsigned(uint64_t value, PrintState* state) {
    // highest possible number of decimal digits =
    // ceil(log10(2^bitlen))
    // = 10 (32bit) or 20 (64bit)

    int printed = 0;

    uint64_t divisor;
    if (state->long_modifier) {
        divisor = 10000000000000000000UL;
    } else {
        const uint32_t int_divisor = 1000000000U;
        divisor = (uint64_t)int_divisor;
    }

    bool started = false;

    // edge case that's easier to handle here
    if (value == 0) {
        putchar('0');
        return 1;
    }

    while (divisor > 0) {
        if (value >= divisor) {
            started = true;
            const uint64_t digit = value / divisor;
            value -= digit * divisor;
            putchar('0' + digit);
            printed += 1;
        } else if (started) {
            putchar('0');
            printed += 1;
        } else {
            // do nothing!
        }

        divisor /= 10;
    }

    return printed;
}

int print_signed(int64_t signed_value, PrintState* state) {
    int printed = 0;
    if (signed_value < 0) {
        putchar('-');
        printed += 1;

        // TODO: just OR all bits except MSB?
        signed_value *= -1;
    }

    printed += print_unsigned(signed_value, state);
    return printed;
}

int printf(const char* format_str, ...) {
    va_list args;
    va_start(args, format_str);

    int printed = 0;

    PrintState state = {0};
    while (*format_str != '\0') {
        if (!state.in_conversion_spec) {
            // if we aren't currently parsing a conversion spec
            if (*format_str == '%') {
                state.in_conversion_spec = true;
            } else {
                putchar(*format_str);
                printed += 1;
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
                    printed += 1;
                    PrintState_reset(&state);
                    break;
                }
                case 'c': {
                    // character
                    int c = va_arg(args, int);
                    putchar(c);
                    printed += 1;
                    PrintState_reset(&state);
                    break;
                }
                case 's': {
                    // string
                    char* str = va_arg(args, char*);
                    printed += print_string(str);
                    PrintState_reset(&state);
                    break;
                }
                case 'x': {
                    // hexadecimal
                    uint64_t value = va_arg(args, uint64_t);
                    printed += print_hex(value, &state);
                    PrintState_reset(&state);
                    break;
                }
                case 'i':
                case 'd': {
                    // signed decimal integer
                    int64_t signed_value = va_arg(args, int64_t);
                    printed += print_signed(signed_value, &state);
                    PrintState_reset(&state);
                    break;
                }
                case 'u': {
                    // unsigned decimal integer
                    uint64_t value = va_arg(args, uint64_t);
                    printed += print_unsigned(value, &state);
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

    return printed;
}
