#include <stddef.h>
#include <stdio.h>

#include "syscall.h"
#include "syscalls.h"

typedef void File;

FILE* stdin;
FILE* stdout;

FILE* fopen(const char* restrict filename, const char* restrict mode) {
    (void)filename;
    (void)mode;
    // TODO
    return NULL;
}
int fclose(FILE* stream) {
    (void)stream;
    // TODO
    return EOF;
}
int fflush(FILE* stream) {
    (void)stream;
    // TODO
    return EOF;
}
int fgetc(FILE* stream) {
    (void)stream;
    // TODO
    return EOF;
}
int fputc(int ch, FILE* stream) {
    // TODO
    (void)stream;
    return SYSCALL(SYSCALL_PUTCHAR, ch);
}
