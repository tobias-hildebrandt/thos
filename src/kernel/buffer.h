#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_LEN 1024

struct Buffer {
    size_t count;
    size_t capacity;
    char* array;
};
typedef struct Buffer Buffer;

Buffer Buffer_wrap(char* backing_array, size_t capacity);
bool Buffer_is_full(Buffer* buffer);
bool Buffer_try_push(Buffer* buffer, char byte);
void Buffer_clear(Buffer* buffer);

void Buffer_output_handle_new(Buffer* buffer, char new);
void Buffer_output_flush(Buffer* buffer);
