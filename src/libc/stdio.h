#pragma once

#include <stdarg.h>
#include <stddef.h>

// NOLINTNEXTLINE(modernize-macro-to-enum)
#define EOF (-1)

// opaque file type
typedef void FILE;

int putchar(int ch);
int getchar(void);

int putc(int ch, FILE* stream);
int getc(FILE* stream);

int printf(const char* restrict format_str, ...);
int vprintf(const char* restrict format_str, va_list vlist);
int fprintf(FILE* stream, const char* restrict format_str, ...);
int vfprintf(FILE* stream, const char* restrict format_str, va_list vlist);
int sprintf(char* restrict buffer, const char* restrict format_str, ...);
int vsprintf(char* restrict buffer, const char* restrict format_str,
             va_list vlist);
int snprintf(char* restrict buffer, size_t bufsz,
             const char* restrict format_str, ...);

// kernel and userlib MUST define these

extern FILE* stdin;
extern FILE* stdout;

extern FILE* fopen(const char* restrict filename, const char* restrict mode);
extern int fclose(FILE* stream);
extern int fflush(FILE* stream);
extern int fgetc(FILE* stream);
extern int fputc(int ch, FILE* stream);
