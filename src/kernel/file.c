#include "file.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "sbi.h"

FILE* stdin = (File*)(&File_stdin);
FILE* stdout = (File*)(&File_stdout);

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
FILE* fopen(const char* restrict filename, const char* restrict mode) {
    if (0 == strcmp(filename, "stdout") && 0 == strcmp(mode, "w")) {
        return stdout;
    } else if (0 == strcmp(filename, "stdin") && 0 == strcmp(mode, "r")) {
        return stdin;
    } else {
        return NULL;
    }
}

int fclose(FILE* _stream) {
    (void)_stream;
    // TODO
    return 0;
}
int fflush(FILE* _stream) {
    (void)_stream;
    // TODO
    return 0;
}

int fgetc(FILE* _stream) {
    File* stream = (File*)_stream;
    if (stream->type == FILETYPE_STDIN) {
        SbiReturn ret = sbi_getchar();
        return (int)ret.value;
    } else {
        return EOF;
    }
}
int fputc(int ch, FILE* _stream) {
    File* stream = (File*)_stream;
    if (stream->type == FILETYPE_STDOUT) {
        SbiReturn ret = sbi_putchar(ch);
        return (int)ret.value;
    } else {
        return EOF;
    }
}
