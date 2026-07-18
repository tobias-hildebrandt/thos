#pragma once

#include <stddef.h>

// TODO: GCC requires the freestanding environment provide
// memcpy, memmove, memset and memcmp
void* memset(void* buf, char value, size_t count);
void* memcpy(void* dst, const void* src, size_t count);

int strcmp(const char* lhs, const char* rhs);
int strncmp(const char* lhs, const char* rhs, size_t count);
