#pragma once

#include <stddef.h>

#include "buffer.h"

enum FileType {
    FILETYPE_STDOUT = 1,
    FILETYPE_STDIN = 2,
    FILETYPE_BUFFER = 3,
};
typedef enum FileType FileType;

struct File {
    FileType type;
    union {
        // valid for types STDOUT and STDIN
        void* nothing;
        // valid for types BUFFER
        Buffer* buffer;
    } data;
};
typedef struct File File;

const File File_stdout = {
    .type = FILETYPE_STDOUT,
    .data = {NULL},
};

const File File_stdin = {
    .type = FILETYPE_STDIN,
    .data = {NULL},
};
