#pragma once

#include "types.h"  // IWYU pragma: keep

#define PAGE_SIZE 4096

extern char __PAGES_START[], __PAGES_END[];

uint64_t alloc_page();
