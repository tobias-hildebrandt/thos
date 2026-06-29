#pragma once

#include "types.h"  // IWYU pragma: keep

#define PAGE_SIZE 4096

uint64_t alloc_page(void);

void enable_virtual_memory(void);
