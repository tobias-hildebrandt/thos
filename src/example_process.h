#pragma once

#include "types.h"  // IWYU pragma: keep

struct SomeData {
    uint64_t d;
    uint32_t w;
    uint8_t b[16];
};
typedef struct SomeData SomeData;

void process_load_s1(void);
void process_load_from_stack(void);
void process_that_returns(void);
void process_mem_ops(void);
