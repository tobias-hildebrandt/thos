#include "sbi.h"
#include "string.h"
#include "types.h"

// TODO:
// https://github.com/riscv-non-isa/riscv-sbi-doc/blob/master/src/ext-debug-console.adoc

void put_char(const char ch) {
    // console putchar
    sbi_call(ch, 0, 0, 0, 0, 0, 0, 1);
}

void put_c_str(const char* str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
        put_char(str[i]);
    }
}

void put_string(const String* string) {
    if (string->len == 0) {
        return;
    }
    for (size_t i = 0; i < string->len - 1; i++) {
        put_char(string->data[i]);
    }
}

void printf(const char* format_str, ...) {
    va_list args;
    va_start(args, format_str);

    while (*format_str) {
        if (*format_str != '%') {
            put_char(*format_str);
        } else {
            // skip %
            format_str += 1;

            switch (*format_str) {
                case '\0':
                    // end of format string
                    put_char('%');
                    goto printf_cleanup;
                case '%':
                    // %%
                    put_char('%');
                    break;
                case 'S': {
                    // %S, String
                    String* string = va_arg(args, String*);
                    put_string(string);
                    break;
                }
                case 's': {
                    // %s, c string
                    char* c_str = va_arg(args, char*);
                    put_c_str(c_str);
                    break;
                }
                case 'x': {
                    // %x, hexadecimal
                    unsigned int value = va_arg(args, unsigned int);
                    for (int i = 8; i > 0; i--) {
                        unsigned int shift_amount = i - 1;
                        unsigned int hex = (value >> (shift_amount * 4)) & 0xf;
                        unsigned int ascii;
                        if (hex <= 9) {
                            ascii = hex + '0';
                        } else {
                            ascii = hex + 'a' - 10;
                        }
                        put_char(ascii);
                    }
                }
                    // TODO: default, error
            }
        }

        // go to next character
        format_str += 1;
    }

printf_cleanup:
    va_end(args);
}
