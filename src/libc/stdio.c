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

struct PrintTarget {
    enum PrintTargetType {
        TARGET_TYPE_BUFFER = 1,
        TARGET_TYPE_STREAM = 2,
    } type;
    union PrintTargetOutput {
        FILE* stream;
        char* restrict buffer;
    } output;
};
typedef struct PrintTarget PrintTarget;

struct MaxOutput {
    bool enabled;
    size_t maximum;
};
typedef struct MaxOutput MaxOutput;

#define NO_MAX_OUTPUT    \
    (MaxOutput) {        \
        .enabled = false \
    }

struct ConversionSpec {
    uint8_t long_modifiers;
    bool zero_pad;
    bool left_pad;
    bool alternative;
    const char* min_width_chars;
    int min_width;
};
typedef struct ConversionSpec ConversionSpec;

struct PrintState {
    bool in_conversion_spec;
    ConversionSpec conversion;
    int printed;
    PrintTarget target;
    va_list arguments;
    MaxOutput max_output;
};
typedef struct PrintState PrintState;

// function pointer that can be passed to min_width_call
typedef void(ConversionPrintFunc)(PrintState* state);

static bool ConversionSpec_value_is_32bit(ConversionSpec* spec) {
    return (POINTER_BITS == 32 && spec->long_modifiers < 2) ||
           (POINTER_BITS == 64 && spec->long_modifiers == 0);
}

// put a character to the target
static void PrintTarget_putchar(PrintTarget* target, int character) {
    if (target->type == TARGET_TYPE_BUFFER) {
        *target->output.buffer = (char)character;
        target->output.buffer += 1;
    } else if (target->type == TARGET_TYPE_STREAM) {
        fputc(character, target->output.stream);
    }
}

static void PrintState_reset(PrintState* state) {
    // new empty state
    PrintState new_state = (PrintState){0};

    // save args and target
    new_state.arguments = state->arguments;
    new_state.target = state->target;
    new_state.max_output = state->max_output;
    new_state.printed = state->printed;

    *state = new_state;
}

// only putchar if wouldn't exceed maximum output
static void putchar_maybe_max(PrintState* state, int character) {
    if (!state->max_output.enabled ||
        ((state->max_output.maximum != 0) &&
         (state->max_output.maximum - 1) > (size_t)state->printed)) {
        PrintTarget_putchar(&state->target, character);
    }
    state->printed += 1;
}

// only putchar if not min_width
// used to calculate length of real print before actually printing
static void putchar_maybe_minwidth(PrintState* state, int character) {
    if (state->conversion.min_width == 0) {
        putchar_maybe_max(state, character);
    } else {
        // pretend we printed one, handled and reset in min_width_call
        state->printed += 1;
    }
}

static void print_char(PrintState* state) {
    int ch = va_arg(state->arguments, int);
    putchar_maybe_minwidth(state, ch);
}

static void print_string(PrintState* state) {
    char* str = va_arg(state->arguments, char*);

    int index = 0;
    for (; str[index] != '\0'; index++) {
        putchar_maybe_minwidth(state, str[index]);
    }
}

static void print_hex(PrintState* state) {
    int shift;
    uint32_t value32 = 0;
    uint64_t value64 = 0;
    // TODO: just use platform's int/long/longlong types?
    bool use_32 = ConversionSpec_value_is_32bit(&state->conversion);
    if (use_32) {
        value32 = va_arg(state->arguments, int32_t);
        shift = 8 - 1;
    } else {
        value64 = va_arg(state->arguments, int64_t);
        shift = 16 - 1;
    }

    if (state->conversion.alternative) {
        putchar_maybe_minwidth(state, '0');
        putchar_maybe_minwidth(state, 'x');
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
        putchar_maybe_minwidth(state, (int)ascii);
    }
}

static void print_binary(PrintState* state) {
    int shift;
    uint32_t value32 = 0;
    uint64_t value64 = 0;
    // TODO: just use platform's int/long/longlong types?
    bool use_32 = ConversionSpec_value_is_32bit(&state->conversion);
    if (use_32) {
        value32 = va_arg(state->arguments, int32_t);
        shift = 32 - 1;
    } else {
        value64 = va_arg(state->arguments, int64_t);
        shift = 64 - 1;
    }

    if (state->conversion.alternative) {
        putchar_maybe_minwidth(state, '0');
        putchar_maybe_minwidth(state, 'b');
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
        putchar_maybe_minwidth(state, ascii);
    }
}

#define DECLARE_PRINT_UNSIGNED_X(type, starting_divisor)               \
    static void print_unsigned_##type(PrintState* state, type value) { \
        type divisor = starting_divisor;                               \
        bool started = false;                                          \
                                                                       \
        /* edge case easier to handle here*/                           \
        if (value == 0) {                                              \
            putchar_maybe_minwidth(state, '0');                        \
            return;                                                    \
        }                                                              \
                                                                       \
        while (divisor > 0) {                                          \
            if (value >= divisor) {                                    \
                started = true;                                        \
                const type digit = value / divisor;                    \
                value -= digit * divisor;                              \
                putchar_maybe_minwidth(state, '0' + digit);            \
            } else if (started) {                                      \
                putchar_maybe_minwidth(state, '0');                    \
            }                                                          \
            divisor /= 10;                                             \
        }                                                              \
    }

DECLARE_PRINT_UNSIGNED_X(uint32_t, 1000000000U)
DECLARE_PRINT_UNSIGNED_X(uint64_t, 10000000000000000000ULL)

static void print_unsigned(PrintState* state) {
    if (!ConversionSpec_value_is_32bit(&state->conversion)) {
        uint64_t value = va_arg(state->arguments, uint64_t);
        print_unsigned_uint64_t(state, value);
    } else {
        uint32_t value = va_arg(state->arguments, uint32_t);
        print_unsigned_uint32_t(state, value);
    }
}

static void print_signed(PrintState* state) {
    if (!ConversionSpec_value_is_32bit(&state->conversion)) {
        int64_t value = va_arg(state->arguments, int64_t);
        if (value < 0) {
            putchar_maybe_minwidth(state, '-');
            value *= -1;
        }
        print_unsigned_uint64_t(state, (uint64_t)value);
    } else {
        int32_t value = va_arg(state->arguments, int32_t);
        if (value < 0) {
            putchar_maybe_minwidth(state, '-');
            value *= -1;
        }
        print_unsigned_uint32_t(state, (uint32_t)value);
    }
}

static void print_padding(PrintState* state, size_t amount) {
    for (size_t i = 0; i < amount; i++) {
        if (state->conversion.zero_pad) {
            putchar_maybe_max(state, '0');
        } else {
            putchar_maybe_max(state, ' ');
        }
    }
}

// TODO: handle +/- sign with zero-padding
static void min_width_call(PrintState* state, ConversionPrintFunc func) {
    if (state->conversion.min_width > 0) {
        // need to pad before or after

        int requested = state->conversion.min_width;
        int old_printed = state->printed;

        if (state->conversion.left_pad) {
            // need to pad beforehand

            // backup arguments
            va_list backup;
            va_copy(backup, state->arguments);

            // "fake" call
            (func)(state);

            // we didn't actually print anything yet
            int fake_printed = state->printed - old_printed;
            state->printed = old_printed;

            // reset real arguments
            va_copy(state->arguments, backup);

            // print padding
            if (requested > fake_printed) {
                print_padding(state, requested - fake_printed);
            }

            // "real" call
            state->conversion.min_width = 0;
            (func)(state);
        } else {
            // need to pad after

            // real call
            state->conversion.min_width = 0;
            (func)(state);

            int new_printed = state->printed - old_printed;

            // fill rest
            if (requested > new_printed) {
                print_padding(state, requested - new_printed);
            }
        }
    } else {
        // no padding, call normally
        (func)(state);
    }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static void handle_conversion_spec(char character, PrintState* state) {
    switch (character) {
        case '\0': {
            // undefined behavior
            // PANIC("end of format string inside of conversion spec");
            break;
        }
        // modifiers
        case '#': {
            // alternative
            state->conversion.alternative = true;
            break;
        }
        case 'l': {
            // long
            state->conversion.long_modifiers += 1;
            break;
        }
        case '*': {
            // variable min width
            int width = va_arg(state->arguments, int);
            state->conversion.min_width = width;
            break;
        }
        case '-': {
            state->conversion.left_pad = true;
            break;
        }

        // conversion specifiers
        case '%': {
            // %%
            putchar_maybe_max(state, '%');
            PrintState_reset(state);
            break;
        }
        case 'c': {
            // character
            min_width_call(state, print_char);
            PrintState_reset(state);
            break;
        }
        case 's': {
            // string
            min_width_call(state, print_string);
            PrintState_reset(state);
            break;
        }
        case 'x': {
            // hexadecimal
            min_width_call(state, print_hex);
            PrintState_reset(state);
            break;
        }
        case 'b': {
            // binary
            min_width_call(state, print_binary);
            PrintState_reset(state);
            break;
        }
        case 'i':
        case 'd': {
            // signed decimal integer
            min_width_call(state, print_signed);
            PrintState_reset(state);
            break;
        }
        case 'u': {
            // unsigned decimal integer
            min_width_call(state, print_unsigned);
            PrintState_reset(state);
            break;
        }
        case 'p': {
            // pointer
            // implementation specific, we can do whatever we want

            if (POINTER_BITS == 64) {
                // "0x%016lx"
                state->conversion.long_modifiers = 1;
                state->conversion.min_width = 16;
            } else {
                // "0x%08x"
                state->conversion.long_modifiers = 0;
                state->conversion.min_width = 8;
            }

            state->conversion.zero_pad = true;
            state->conversion.left_pad = true;

            putchar_maybe_max(state, '0');
            putchar_maybe_max(state, 'x');

            min_width_call(state, print_hex);
            PrintState_reset(state);
            break;
        }
        default: {
            // PANIC("unknown conversion specification character: %c",
            //       *format_str);
        }
    }
}

// common function for entire {vsnf}*printf family
static int PrintTarget_do_print(PrintTarget target, const char* format_str,
                                va_list vlist, MaxOutput max_output) {
    PrintState state = {0};

    state.target = target;
    state.arguments = vlist;
    state.max_output = max_output;

    while (*format_str != '\0') {
        if (!state.in_conversion_spec) {
            // if we aren't currently parsing a conversion spec

            if (*format_str == '%') {
                state.in_conversion_spec = true;
            } else {
                putchar_maybe_max(&state, *format_str);
            }
        } else {
            if (state.conversion.zero_pad == false &&
                state.conversion.min_width_chars == NULL &&
                *format_str == '0') {
                // zero-pad

                state.conversion.zero_pad = true;

                // done parsing this character
                format_str += 1;
                continue;
            }

            if (*format_str >= '0' && *format_str <= '9') {
                // min width

                if (state.conversion.min_width_chars == NULL) {
                    // start of min width string
                    state.conversion.min_width_chars = format_str;
                } else {
                    // middle of min width string
                }

                // done parsing this character
                format_str += 1;
                continue;
            } else if (state.conversion.min_width_chars != NULL) {
                // end of min width string
                state.conversion.min_width =
                    atoi(state.conversion.min_width_chars);
                state.conversion.min_width_chars = NULL;

                // keep parsing this char
            }

            // we are currently parsing a conversion spec
            handle_conversion_spec(*format_str, &state);
        }

        // go to next character
        format_str += 1;
    }

    return state.printed;
}

int printf(const char* restrict format_str, ...) {
    va_list args;
    va_start(args, format_str);

    PrintTarget target =
        (PrintTarget){.type = TARGET_TYPE_STREAM, .output.stream = stdout};

    int printed = PrintTarget_do_print(target, format_str, args, NO_MAX_OUTPUT);

    va_end(args);

    return printed;
}

int vprintf(const char* restrict format_str, va_list vlist) {
    PrintTarget target =
        (PrintTarget){.type = TARGET_TYPE_STREAM, .output.stream = stdout};

    int printed =
        PrintTarget_do_print(target, format_str, vlist, NO_MAX_OUTPUT);

    return printed;
}

int fprintf(FILE* stream, const char* restrict format_str, ...) {
    va_list args;
    va_start(args, format_str);

    PrintTarget target =
        (PrintTarget){.type = TARGET_TYPE_STREAM, .output.stream = stream};

    int printed = PrintTarget_do_print(target, format_str, args, NO_MAX_OUTPUT);

    va_end(args);

    return printed;
}

int vfprintf(FILE* stream, const char* restrict format_str, va_list vlist) {
    PrintTarget target =
        (PrintTarget){.type = TARGET_TYPE_STREAM, .output.stream = stream};

    int printed =
        PrintTarget_do_print(target, format_str, vlist, NO_MAX_OUTPUT);

    return printed;
}

int sprintf(char* restrict buffer, const char* restrict format_str, ...) {
    va_list args;
    va_start(args, format_str);

    PrintTarget target =
        (PrintTarget){.type = TARGET_TYPE_BUFFER, .output.buffer = buffer};

    int printed = PrintTarget_do_print(target, format_str, args, NO_MAX_OUTPUT);

    // write nul terminator
    buffer[printed] = 0;

    va_end(args);

    return printed;
}

int vsprintf(char* restrict buffer, const char* restrict format_str,
             va_list vlist) {
    PrintTarget target =
        (PrintTarget){.type = TARGET_TYPE_BUFFER, .output.buffer = buffer};

    int printed =
        PrintTarget_do_print(target, format_str, vlist, NO_MAX_OUTPUT);

    // write nul terminator
    buffer[printed] = 0;

    return printed;
}

int snprintf(char* restrict buffer, size_t bufsz,
             const char* restrict format_str, ...) {
    va_list args;
    va_start(args, format_str);

    PrintTarget target =
        (PrintTarget){.type = TARGET_TYPE_BUFFER, .output.buffer = buffer};

    MaxOutput max_output = {.enabled = true, .maximum = bufsz};

    int printed = PrintTarget_do_print(target, format_str, args, max_output);

    // write nul terminator
    if (bufsz > (size_t)printed) {
        // write at end of written string
        buffer[printed] = 0;
    } else if (bufsz == 0) {
        // don't write anything
    } else {
        // write at max size index
        buffer[bufsz - 1] = 0;
    }

    va_end(args);

    return printed;
}
