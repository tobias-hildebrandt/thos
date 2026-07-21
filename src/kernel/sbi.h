#pragma once

#include <stdint.h>
struct SbiReturn {
    long error;
    long value;
};
typedef struct SbiReturn SbiReturn;

SbiReturn sbi_putchar(int ch);
SbiReturn sbi_shutdown(long type);
SbiReturn sbi_set_timer(uint64_t time);
SbiReturn sbi_hart_start(unsigned long hart_id, unsigned long start_addr,
                         unsigned long opaque);
SbiReturn sbi_hart_stop(void);
