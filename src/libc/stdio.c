#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "build_info.h"

// https://en.cppreference.com/c/io/fprintf
// %[modifiers][min width][.precision][type length specifier]conversion_format

#define SHOULD_USE_32(STATE)                             \
    (POINTER_BITS == 32 && STATE->long_modifiers < 2) || \
        (POINTER_BITS == 64 && STATE->long_modifiers == 0)

struct PrintState {
    bool in_conversion_spec;
    uint8_t long_modifiers;
    bool zero_pad;
    bool left_pad;
    bool alternative;
    const char* min_width_chars;
    int min_width;
};
typedef struct PrintState PrintState;

void PrintState_reset(PrintState* state) {
    *state = (PrintState){0};
}

bool PrintState_is_clean(PrintState* state) {
    return state->long_modifiers == 0 && state->zero_pad == 0 &&
           state->min_width == 0;
}

// only putchar if not min_width
// used to calculate length of real print before actually printing
void maybe_putchar(int ch, PrintState* state) {
    if (state->min_width == 0) {
        putchar(ch);
    }
}

int print_char(int ch, PrintState* state) {
    maybe_putchar(ch, state);
    return 1;
}

int print_string(char* str, PrintState* state) {
    int i = 0;
    for (; str[i] != '\0'; i++) {
        maybe_putchar(str[i], state);
    }

    return i;
}

int print_hex(va_list* args, PrintState* state) {
    int printed = 0;
    int shift;
    uint32_t value32 = 0;
    uint64_t value64 = 0;
    // TODO: just use platform's int/long/longlong types?
    bool use_32 = SHOULD_USE_32(state);
    if (use_32) {
        value32 = va_arg(*args, int32_t);
        shift = 8 - 1;
    } else {
        value64 = va_arg(*args, int64_t);
        shift = 16 - 1;
    }

    if (state->alternative) {
        maybe_putchar('0', state);
        maybe_putchar('x', state);
        printed += 2;
    }

    bool started = false;

    for (; shift >= 0; shift--) {
        uint64_t place_value =
            ((use_32 ? value32 : value64) >> (shift * 4)) & 0xf;

        if (place_value == 0 && !started && shift != 0) {
            continue;
        }

        unsigned int ascii;
        if (place_value <= 9) {
            ascii = '0' + place_value;
        } else {
            ascii = 'a' + place_value - 10;
        }
        started = true;
        maybe_putchar(ascii, state);
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
        /* edge case easier to handle here*/                   \
        if (value == 0) {                                      \
            maybe_putchar('0', state);                         \
            return 1;                                          \
        }                                                      \
                                                               \
        while (divisor > 0) {                                  \
            if (value >= divisor) {                            \
                started = true;                                \
                const type digit = value / divisor;            \
                value -= digit * divisor;                      \
                maybe_putchar('0' + digit, state);             \
                printed += 1;                                  \
            } else if (started) {                              \
                maybe_putchar('0', state);                     \
                printed += 1;                                  \
            }                                                  \
            divisor /= 10;                                     \
        }                                                      \
        return printed;                                        \
    }

DECLARE_PRINT_UNSIGNED_X(uint32_t, 1000000000U)
DECLARE_PRINT_UNSIGNED_X(uint64_t, 10000000000000000000ULL)

int print_unsigned(va_list* args, PrintState* state) {
    if (!SHOULD_USE_32(state)) {
        uint64_t value = va_arg(*args, uint64_t);
        return print_unsigned_uint64_t(value, state);
    } else {
        uint32_t value = va_arg(*args, uint32_t);
        return print_unsigned_uint32_t(value, state);
    }
}

int print_signed(va_list* args, PrintState* state) {
    int printed = 0;

    if (!SHOULD_USE_32(state)) {
        int64_t value = va_arg(*args, int64_t);
        if (value < 0) {
            maybe_putchar('-', state);
            printed += 1;
            value *= -1;
        }
        printed += print_unsigned_uint64_t((uint64_t)value, state);
    } else {
        int32_t value = va_arg(*args, int32_t);
        if (value < 0) {
            maybe_putchar('-', state);
            printed += 1;
            value *= -1;
        }
        printed += print_unsigned_uint32_t((uint32_t)value, state);
    }

    return printed;
}

// TODO: handle +/- sign with zero-padding
// TODO: handle right-align
#define MIN_WIDTH_CALL(STATE, PRINTED, FAKE_CALL, REAL_CALL)       \
    do {                                                           \
        if (STATE.min_width > 0 && !STATE.left_pad) {              \
            int requested = STATE.min_width;                       \
            /* call with real min_width, doesn't actually print */ \
            int would_print = FAKE_CALL;                           \
            if (requested > would_print) {                         \
                int to_print = requested - would_print;            \
                for (int i = 0; i < to_print; i++) {               \
                    if (STATE.zero_pad) {                          \
                        putchar('0');                              \
                    } else {                                       \
                        putchar(' ');                              \
                    }                                              \
                    PRINTED += 1;                                  \
                }                                                  \
            }                                                      \
            /* call with min_width 0, will print */                \
            STATE.min_width = 0;                                   \
            PRINTED += REAL_CALL;                                  \
        } else if (STATE.min_width > 0 && STATE.left_pad) {        \
            int requested = STATE.min_width;                       \
            /* call with min_width 0, will print */                \
            STATE.min_width = 0;                                   \
            int did_print = REAL_CALL;                             \
            PRINTED += did_print;                                  \
            if (requested > did_print) {                           \
                int to_print = requested - did_print;              \
                for (int i = 0; i < to_print; i++) {               \
                    if (STATE.zero_pad) {                          \
                        putchar('0');                              \
                    } else {                                       \
                        putchar(' ');                              \
                    }                                              \
                    PRINTED += 1;                                  \
                }                                                  \
            }                                                      \
        } else {                                                   \
            PRINTED += REAL_CALL;                                  \
        }                                                          \
    } while (0)

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
            // zero-pad
            if (state.zero_pad == false && state.min_width_chars == NULL &&
                *format_str == '0') {
                state.zero_pad = true;

                // done parsing this character
                format_str += 1;
                continue;
            }

            // min width
            if (*format_str >= '0' && *format_str <= '9') {
                if (state.min_width_chars == NULL) {
                    // start of min width string
                    state.min_width_chars = format_str;
                } else {
                    // middle of min width string
                }

                // done parsing this character
                format_str += 1;
                continue;
            } else if (state.min_width_chars != NULL) {
                // end of min width string
                state.min_width = atoi(state.min_width_chars);
                state.min_width_chars = NULL;

                // keep parsing this char
            }

            // we are currently parsing a conversion spec
            switch (*format_str) {
                case '\0': {
                    // undefined behavior
                    // PANIC("end of format string inside of conversion spec");
                    break;
                }
                // modifiers
                case '#': {
                    // alternative
                    state.alternative = true;
                    break;
                }
                case 'l': {
                    // long
                    state.long_modifiers += 1;
                    break;
                }
                case '*': {
                    // variable min width
                    int i = va_arg(args, int);
                    state.min_width = i;
                    break;
                }
                case '-': {
                    state.left_pad = true;
                    break;
                }
                // conversion specifiers
                case '%': {
                    // %%
                    // if (!PrintState_is_clean(&state)) {
                    //     PANIC(
                    //         "printf conversion specifier was %%, but had "
                    //         "modifiers");
                    // }
                    putchar('%');
                    printed += 1;
                    PrintState_reset(&state);
                    break;
                }
                case 'c': {
                    // character
                    int ch = va_arg(args, int);
                    MIN_WIDTH_CALL(state, printed, print_char(ch, &state),
                                   print_char(ch, &state));
                    PrintState_reset(&state);
                    break;
                }
                case 's': {
                    // string
                    char* str = va_arg(args, char*);
                    MIN_WIDTH_CALL(state, printed, print_string(str, &state),
                                   print_string(str, &state));
                    PrintState_reset(&state);
                    break;
                }
                case 'x': {
                    // hexadecimal
                    va_list args_copy;
                    va_copy(args_copy, args);
                    MIN_WIDTH_CALL(state, printed,
                                   print_hex(&args_copy, &state),
                                   print_hex(&args, &state));
                    PrintState_reset(&state);
                    break;
                }
                case 'i':
                case 'd': {
                    // signed decimal integer
                    va_list args_copy;
                    va_copy(args_copy, args);
                    MIN_WIDTH_CALL(state, printed,
                                   print_signed(&args_copy, &state),
                                   print_signed(&args, &state));
                    PrintState_reset(&state);
                    break;
                }
                case 'u': {
                    // unsigned decimal integer
                    va_list args_copy;
                    va_copy(args_copy, args);
                    MIN_WIDTH_CALL(state, printed,
                                   print_unsigned(&args_copy, &state),
                                   print_unsigned(&args, &state));
                    PrintState_reset(&state);
                    break;
                }
                case 'p': {
                    // pointer
                    // implementation specific, we can do whatever we want

                    if (POINTER_BITS == 64) {
                        // "0x%016lx"
                        state.long_modifiers = 1;
                        state.min_width = 16;
                    } else {
                        // "0x%08x"
                        state.long_modifiers = 0;
                        state.min_width = 8;
                    }

                    state.zero_pad = true;

                    putchar('0');
                    putchar('x');
                    printed += 2;

                    va_list args_copy;
                    va_copy(args_copy, args);
                    MIN_WIDTH_CALL(state, printed,
                                   print_hex(&args_copy, &state),
                                   print_hex(&args, &state));
                    PrintState_reset(&state);
                    break;
                }
                default: {
                    // PANIC("unknown conversion specification character: %c",
                    //       *format_str);
                }
            }
        }

        // go to next character
        format_str += 1;
    }

    va_end(args);

    return printed;
}
