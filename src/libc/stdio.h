#pragma once

int printf(const char* format_str, ...);

// kernel and userlib MUST define this
extern int putchar(int ch);

#define EOF (-1)
