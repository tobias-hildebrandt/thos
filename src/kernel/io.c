#include <stdio.h>

#include "sbi.h"

// implements <stdio.h>'s putchar
int putchar(int character) {
    unsigned char real_char = (unsigned char)character;
    SbiReturn ret = sbi_putchar(real_char);
    if (ret.error != 0) {
        return EOF;
    } else {
        return character;
    }
}
