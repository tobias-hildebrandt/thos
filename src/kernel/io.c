#include <stdio.h>

#include "sbi.h"

// implements <stdio.h>'s putchar
int putchar(int ch) {
    unsigned char real_char = (unsigned char)ch;
    SbiReturn ret = sbi_putchar(real_char);
    if (ret.error != 0) {
        return EOF;
    } else {
        return ch;
    }
}
