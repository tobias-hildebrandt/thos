#pragma once

#include <stddef.h>

void* memset(void* buf, char c, size_t n);
void* memcpy(void* dst, const void* src, size_t n);

int strcmp(const char* lhs, const char* rhs);
