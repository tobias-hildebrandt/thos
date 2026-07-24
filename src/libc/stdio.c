#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "build_info.h"

// TODO: implement half modifiers %h

// https://en.cppreference.com/c/io/fprintf
// %[modifiers][min width][.precision][type length specifier]conversion_format

#define SHOULD_USE_32(STATE)                               \
    (POINTER_BITS == 32 && (STATE)->long_modifiers < 2) || \
        (POINTER_BITS == 64 && (STATE)->long_modifiers == 0)

int putchar(int character) {
    return fputc(character, stdout);
}

int getchar(void) {
    return fgetc(stdin);
}

int putc(int ch, FILE* stream) {
    return fputc(ch, stream);
}

int getc(FILE* stream) {
    return fgetc(stream);
}

struct PrintState {
    bool in_conversion_spec;
    uint8_t long_modifiers;
    bool zero_pad;
    bool left_pad;
    bool alternative;
    const char* min_width_chars;
    int min_width;
    FILE* stream;
    va_list arguments;
};
typedef struct PrintState PrintState;

typedef int(ConversionPrintFunc)(PrintState* state);

static void PrintState_reset(PrintState* state) {
    // save args and stream
    va_list arguments = state->arguments;
    FILE* stream = state->stream;

    *state = (PrintState){0};

    state->arguments = arguments;
    state->stream = stream;
}

// only putchar if not min_width
// used to calculate length of real print before actually printing
static void maybe_putchar(int character, PrintState* state) {
    if (state->min_width == 0) {
        fputc(character, state->stream);
    }
}

static int print_char(PrintState* state) {
    int ch = va_arg(state->arguments, int);
    maybe_putchar(ch, state);
    return 1;
}

static int print_string(PrintState* state) {
    char* str = va_arg(state->arguments, char*);

    int index = 0;
    for (; str[index] != '\0'; index++) {
        maybe_putchar(str[index], state);
    }

    return index;
}

static int print_hex(PrintState* state) {
    int printed = 0;
    int shift;
    uint32_t value32 = 0;
    uint64_t value64 = 0;
    // TODO: just use platform's int/long/longlong types?
    bool use_32 = SHOULD_USE_32(state);
    if (use_32) {
        value32 = va_arg(state->arguments, int32_t);
        shift = 8 - 1;
    } else {
        value64 = va_arg(state->arguments, int64_t);
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
        maybe_putchar((int)ascii, state);
        printed += 1;
    }

    return printed;
}

static int print_binary(PrintState* state) {
    int printed = 0;
    int shift;
    uint32_t value32 = 0;
    uint64_t value64 = 0;
    // TODO: just use platform's int/long/longlong types?
    bool use_32 = SHOULD_USE_32(state);
    if (use_32) {
        value32 = va_arg(state->arguments, int32_t);
        shift = 32 - 1;
    } else {
        value64 = va_arg(state->arguments, int64_t);
        shift = 64 - 1;
    }

    if (state->alternative) {
        maybe_putchar('0', state);
        maybe_putchar('b', state);
        printed += 2;
    }

    bool started = false;

    for (; shift >= 0; shift--) {
        uint64_t place_value = ((use_32 ? value32 : value64) >> shift) & 0x1;

        if (place_value == 0 && !started && shift != 0) {
            continue;
        }

        int ascii;
        if (place_value) {
            ascii = '1';
        } else {
            ascii = '0';
        }
        started = true;
        maybe_putchar(ascii, state);
        printed += 1;
    }

    return printed;
}

#define DECLARE_PRINT_UNSIGNED_X(type, starting_divisor)              \
    static int print_unsigned_##type(type value, PrintState* state) { \
        int printed = 0;                                              \
        type divisor = starting_divisor;                              \
        bool started = false;                                         \
                                                                      \
        /* edge case easier to handle here*/                          \
        if (value == 0) {                                             \
            maybe_putchar('0', state);                                \
            return 1;                                                 \
        }                                                             \
                                                                      \
        while (divisor > 0) {                                         \
            if (value >= divisor) {                                   \
                started = true;                                       \
                const type digit = value / divisor;                   \
                value -= digit * divisor;                             \
                maybe_putchar('0' + digit, state);                    \
                printed += 1;                                         \
            } else if (started) {                                     \
                maybe_putchar('0', state);                            \
                printed += 1;                                         \
            }                                                         \
            divisor /= 10;                                            \
        }                                                             \
        return printed;                                               \
    }

DECLARE_PRINT_UNSIGNED_X(uint32_t, 1000000000U)
DECLARE_PRINT_UNSIGNED_X(uint64_t, 10000000000000000000ULL)

static int print_unsigned(PrintState* state) {
    if (!SHOULD_USE_32(state)) {
        uint64_t value = va_arg(state->arguments, uint64_t);
        return print_unsigned_uint64_t(value, state);
    } else {
        uint32_t value = va_arg(state->arguments, uint32_t);
        return print_unsigned_uint32_t(value, state);
    }
}

static int print_signed(PrintState* state) {
    int printed = 0;

    if (!SHOULD_USE_32(state)) {
        int64_t value = va_arg(state->arguments, int64_t);
        if (value < 0) {
            maybe_putchar('-', state);
            printed += 1;
            value *= -1;
        }
        printed += print_unsigned_uint64_t((uint64_t)value, state);
    } else {
        int32_t value = va_arg(state->arguments, int32_t);
        if (value < 0) {
            maybe_putchar('-', state);
            printed += 1;
            value *= -1;
        }
        printed += print_unsigned_uint32_t((uint32_t)value, state);
    }

    return printed;
}

static int print_padding(PrintState* state, int amount) {
    int printed = 0;
    for (int i = 0; i < amount; i++) {
        if (state->zero_pad) {
            fputc('0', state->stream);
        } else {
            fputc(' ', state->stream);
        }
        printed += 1;
    }
    return printed;
}

// TODO: handle +/- sign with zero-padding
static int min_width_call(PrintState* state, ConversionPrintFunc func) {
    int printed = 0;
    if (state->min_width > 0) {
        // need to pad before or after

        int requested = state->min_width;

        if (state->left_pad) {
            // need to pad beforehand

            // backup arguments
            va_list backup;
            va_copy(backup, state->arguments);

            // "fake" call
            int would_print = (func)(state);

            // reset real arguments
            va_copy(state->arguments, backup);

            // print padding
            if (requested > would_print) {
                printed += print_padding(state, requested - would_print);
            }

            // "real" call
            state->min_width = 0;
            printed += (func)(state);
        } else {
            // need to pad after

            // call normally
            printed += (func)(state);

            // fill rest
            if (requested > printed) {
                printed += print_padding(state, requested - printed);
            }
        }
    } else {
        // no padding
        printed += (func)(state);
    }
    return printed;
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static void handle_conversion_spec(char character, PrintState* state,
                                   int* const printed) {
    switch (character) {
        case '\0': {
            // undefined behavior
            // PANIC("end of format string inside of conversion spec");
            break;
        }
        // modifiers
        case '#': {
            // alternative
            state->alternative = true;
            break;
        }
        case 'l': {
            // long
            state->long_modifiers += 1;
            break;
        }
        case '*': {
            // variable min width
            int width = va_arg(state->arguments, int);
            state->min_width = width;
            break;
        }
        case '-': {
            state->left_pad = true;
            break;
        }

        // conversion specifiers
        case '%': {
            // %%
            fputc('%', state->stream);
            *printed += 1;
            PrintState_reset(state);
            break;
        }
        case 'c': {
            // character
            *printed += min_width_call(state, print_char);
            PrintState_reset(state);
            break;
        }
        case 's': {
            // string
            *printed += min_width_call(state, print_string);
            PrintState_reset(state);
            break;
        }
        case 'x': {
            // hexadecimal
            *printed += min_width_call(state, print_hex);
            PrintState_reset(state);
            break;
        }
        case 'b': {
            // binary
            *printed += min_width_call(state, print_binary);
            PrintState_reset(state);
            break;
        }
        case 'i':
        case 'd': {
            // signed decimal integer
            *printed += min_width_call(state, print_signed);
            PrintState_reset(state);
            break;
        }
        case 'u': {
            // unsigned decimal integer
            *printed += min_width_call(state, print_unsigned);
            PrintState_reset(state);
            break;
        }
        case 'p': {
            // pointer
            // implementation specific, we can do whatever we want

            if (POINTER_BITS == 64) {
                // "0x%016lx"
                state->long_modifiers = 1;
                state->min_width = 16;
            } else {
                // "0x%08x"
                state->long_modifiers = 0;
                state->min_width = 8;
            }

            state->zero_pad = true;

            fputc('0', state->stream);
            fputc('x', state->stream);
            *printed += 2;

            *printed += min_width_call(state, print_hex);
            PrintState_reset(state);
            break;
        }
        default: {
            // PANIC("unknown conversion specification character: %c",
            //       *format_str);
        }
    }
}

static int vfprintf_ref(FILE* stream, const char* format_str, va_list vlist) {
    int printed = 0;

    PrintState state = {0};

    state.stream = stream;
    state.arguments = vlist;

    while (*format_str != '\0') {
        if (!state.in_conversion_spec) {
            // if we aren't currently parsing a conversion spec

            if (*format_str == '%') {
                state.in_conversion_spec = true;
            } else {
                fputc(*format_str, state.stream);
                printed += 1;
            }
        } else {
            if (state.zero_pad == false && state.min_width_chars == NULL &&
                *format_str == '0') {
                // zero-pad

                state.zero_pad = true;

                // done parsing this character
                format_str += 1;
                continue;
            }

            if (*format_str >= '0' && *format_str <= '9') {
                // min width

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
            handle_conversion_spec(*format_str, &state, &printed);
        }

        // go to next character
        format_str += 1;
    }

    return printed;
}

int printf(const char* format_str, ...) {
    va_list args;
    va_start(args, format_str);

    int printed = vfprintf_ref(stdout, format_str, args);

    va_end(args);

    return printed;
}

int fprintf(FILE* stream, const char* format_str, ...) {
    va_list args;
    va_start(args, format_str);

    int printed = vfprintf_ref(stream, format_str, args);

    va_end(args);

    return printed;
}

int vfprintf(FILE* stream, const char* format_str, va_list vlist) {
    int printed = vfprintf_ref(stream, format_str, vlist);

    return printed;
}
