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
    bool alternative;
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

int print_hex(va_list* args, PrintState* state) {
    int printed = 0;
    int shift;
    uint32_t int_value;
    uint64_t long_value;
    if (state->long_modifier) {
        long_value = va_arg(*args, int64_t);
        shift = 16 - 1;
    } else {
        int_value = va_arg(*args, int32_t);
        shift = 8 - 1;
    }

    if (state->alternative) {
        putchar('0');
        putchar('x');
        printed += 2;
    }

    for (; shift >= 0; shift--) {
        unsigned int place_value =
            ((state->long_modifier ? long_value : int_value) >> (shift * 4)) &
            0xf;
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

#define DECLARE_PRINT_UNSIGNED_X(type, starting_divisor)       \
    int print_unsigned_##type(type value, PrintState* state) { \
        (void)state;                                           \
        int printed = 0;                                       \
        type divisor = starting_divisor;                       \
        bool started = false;                                  \
                                                               \
        if (value == 0) {                                      \
            putchar('0');                                      \
            return 1;                                          \
        }                                                      \
        while (divisor > 0) {                                  \
            if (value >= divisor) {                            \
                started = true;                                \
                const type digit = value / divisor;            \
                value -= digit * divisor;                      \
                putchar('0' + digit);                          \
                printed += 1;                                  \
            } else if (started) {                              \
                putchar('0');                                  \
                printed += 1;                                  \
            }                                                  \
            divisor /= 10;                                     \
        }                                                      \
        return printed;                                        \
    }

DECLARE_PRINT_UNSIGNED_X(uint32_t, 1000000000U)
DECLARE_PRINT_UNSIGNED_X(uint64_t, 10000000000000000000UL)

int print_unsigned(va_list* args, PrintState* state) {
    if (state->long_modifier) {
        uint64_t value = va_arg(*args, uint64_t);
        return print_unsigned_uint64_t(value, state);
    } else {
        uint32_t value = va_arg(*args, uint32_t);
        return print_unsigned_uint32_t(value, state);
    }
}

int print_signed(va_list* args, PrintState* state) {
    int printed = 0;

    if (state->long_modifier) {
        int64_t value = va_arg(*args, int64_t);
        if (value < 0) {
            putchar('-');
            printed += 1;
            value *= -1;
        }
        printed += print_unsigned_uint64_t((uint64_t)value, state);
    } else {
        int32_t value = va_arg(*args, int32_t);
        if (value < 0) {
            putchar('-');
            printed += 1;
            value *= -1;
        }
        printed += print_unsigned_uint32_t((uint32_t)value, state);
    }

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
                case '#': {
                    // alternative
                    state.alternative = true;
                    break;
                }
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
                    printed += print_hex(&args, &state);
                    PrintState_reset(&state);
                    break;
                }
                case 'i':
                case 'd': {
                    // signed decimal integer
                    printed += print_signed(&args, &state);
                    PrintState_reset(&state);
                    break;
                }
                case 'u': {
                    // unsigned decimal integer
                    printed += print_unsigned(&args, &state);
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
