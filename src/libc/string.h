#pragma once

#include <stddef.h>

// TODO: GCC requires the freestanding environment provide
// memcpy, memmove, memset and memcmp
void* memset(void* buf, char c, size_t n);
void* memcpy(void* dst, const void* src, size_t n);

int strcmp(const char* lhs, const char* rhs);
int strncmp(const char* lhs, const char* rhs, size_t count);
