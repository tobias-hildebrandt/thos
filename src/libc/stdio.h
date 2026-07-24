#pragma once

#include <stdarg.h>

// NOLINTNEXTLINE(modernize-macro-to-enum)
#define EOF (-1)

// opaque file type
typedef void FILE;

int putchar(int ch);
int getchar(void);

int putc(int ch, FILE* stream);
int getc(FILE* stream);

int printf(const char* format_str, ...);
int fprintf(FILE* stream, const char* format_str, ...);
int vfprintf(FILE* stream, const char* format_str, va_list vlist);

// kernel and userlib MUST define these

extern FILE* stdin;
extern FILE* stdout;

extern FILE* fopen(const char* restrict filename, const char* restrict mode);
extern int fclose(FILE* stream);
extern int fflush(FILE* stream);
extern int fgetc(FILE* stream);
extern int fputc(int ch, FILE* stream);
