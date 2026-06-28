#pragma once

#include "types.h"  // IWYU pragma: keep, needed for size_t

// TODO: no need for true c string with nul terminator?

#define STRING(STRING)                        \
    (String) {                                \
        .data = STRING, .len = sizeof(STRING) \
    }

struct String {
    char* data;
    size_t len;
};
typedef struct String String;
