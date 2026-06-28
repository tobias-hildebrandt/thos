#include "sbi.h"
#include "string.h"
#include "types.h"

void put_char(const char ch) {
    sbi_putchar(ch);
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
                    break;
                }
                case 'i':
                case 'd': {
                    // signed decimal integer (NOT LONG)
                    // highest possible number of decimal digits =
                    // ceiling(log10(2^31))
                    // = 10
                    int signed_value = va_arg(args, int);
                    if (signed_value < 0) {
                        put_char('-');
                        // TODO: just OR all bits except MSB?
                        signed_value *= -1;
                    }
                    unsigned int value = (int)signed_value;
                    bool started = false;

                    // edge case that's easier to handle here
                    if (value == 0) {
                        put_char('0');
                        break;
                    }

                    // 10^9
                    unsigned int divisor = 1000000000;
                    while (divisor > 0) {
                        if (value >= divisor) {
                            started = true;
                            const unsigned int digit = value / divisor;
                            value -= digit * divisor;
                            put_char('0' + digit);
                        } else if (started) {
                            put_char('0');
                        } else {
                            // do nothing!
                        }

                        divisor /= 10;
                    }
                    break;
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
