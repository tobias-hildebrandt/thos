#include "io.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "buffer.h"
#include "hart.h"
#include "lock.h"
#include "sbi.h"

static SpinLock sbi_putchar_spinlock;

// TODO: lock at each printf instead of putchar

int putchar_buffer(int character, Buffer* buffer) {
    // may enter with interrupts enabled OR disabled, so we first acquire a
    // spinlock, which disables interrupts

    bool were_interrupts_enabled = SpinLock_acquire(&sbi_putchar_spinlock);

    if (buffer == NULL) {
        // direct to hart's buffer or process buffer
        buffer = my_hart_or_process_output_buffer(were_interrupts_enabled);
    }

    Buffer_output_handle_new(buffer, character);

    SpinLock_release(&sbi_putchar_spinlock);

    return character;
}

int putchar(int character) {
    return putchar_buffer(character, NULL);
}

// TODO: deduplicate from stdio

void put_direct_str(const char* str) {
    while (*str != 0) {
        sbi_putchar(*str);
        str += 1;
    }
}
void put_direct_hex32(uint32_t val) {
    int shift = 8 - 1;

    sbi_putchar('0');
    sbi_putchar('x');

    bool started = false;

    for (; shift >= 0; shift--) {
        uint64_t place_value = (val >> (shift * 4)) & 0xf;

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
        sbi_putchar((int)ascii);
    }
}

void put_direct_u32(uint32_t val) {
    if (val == 0) {
        sbi_putchar('0');
        return;
    }
    uint32_t div = 1000000000U;

    bool started = false;
    while (div > 0) {
        if (val >= div) {
            started = true;
            const uint32_t digit = val / div;
            if (digit > 9) {
                continue;
            }

            val -= digit * div;
            sbi_putchar('0' + (int)digit);
        } else {
            if (started) {
                sbi_putchar('0');
            }
        };

        div /= 10;
    }
}
