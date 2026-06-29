#pragma once

#include "types.h"  // IWYU pragma: keep, needed for size_t

void* memset(void* buf, char c, size_t n);
void* memcpy(void* dst, const void* src, size_t n);
