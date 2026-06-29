#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

void printf(const char* format_str, ...) {
    va_list args;
    va_start(args, format_str);

    while (*format_str) {
        if (*format_str != '%') {
            putchar(*format_str);
        } else {
            // skip %
            format_str += 1;

            switch (*format_str) {
                case '\0':
                    // end of format string
                    putchar('%');
                    goto printf_cleanup;
                case '%':
                    // %%
                    putchar('%');
                    break;
                case 's': {
                    // %s, c string
                    char* c_str = va_arg(args, char*);
                    for (size_t i = 0; c_str[i] != '\0'; i++) {
                        putchar(c_str[i]);
                    }
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
                        putchar(ascii);
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
                        putchar('-');
                        // TODO: just OR all bits except MSB?
                        signed_value *= -1;
                    }
                    unsigned int value = (int)signed_value;
                    bool started = false;

                    // edge case that's easier to handle here
                    if (value == 0) {
                        putchar('0');
                        break;
                    }

                    // 10^9
                    unsigned int divisor = 1000000000;
                    while (divisor > 0) {
                        if (value >= divisor) {
                            started = true;
                            const unsigned int digit = value / divisor;
                            value -= digit * divisor;
                            putchar('0' + digit);
                        } else if (started) {
                            putchar('0');
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
