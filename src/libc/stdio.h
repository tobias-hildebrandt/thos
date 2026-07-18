#pragma once

int printf(const char* format_str, ...);

// kernel and userlib MUST define this
extern int putchar(int character);

// NOLINTNEXTLINE(modernize-macro-to-enum)
#define EOF (-1)
