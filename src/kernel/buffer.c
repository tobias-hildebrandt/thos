#include "buffer.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

Buffer Buffer_wrap(char* backing_array, size_t capacity) {
    memset(backing_array, 0, capacity);

    return (Buffer){
        .count = 0,
        .capacity = capacity,
        .array = backing_array,
    };
}

bool Buffer_is_full(Buffer* buffer) {
    return buffer->count == buffer->capacity;
}

bool Buffer_try_push(Buffer* buffer, char byte) {
    if (Buffer_is_full(buffer)) {
        return false;
    }

    buffer->array[buffer->count] = byte;
    buffer->count += 1;

    return true;
}

void Buffer_clear(Buffer* buffer) {
    memset(buffer->array, 0, buffer->count);

    buffer->count = 0;
}

// flush and clear buffer
void Buffer_output_flush(Buffer* buffer) {
    for (size_t i = 0; i < buffer->count; i++) {
        putchar(buffer->array[i]);
    }
    Buffer_clear(buffer);
}

// if buffer if full or new character is a newline, putchar() everything in
// buffer
void Buffer_output_handle_new(Buffer* buffer, char new) {
    if (new == '\n') {
        Buffer_output_flush(buffer);
        putchar(new);
    } else {
        bool pushed = Buffer_try_push(buffer, new);

        if (!pushed) {
            // buffer is full
            Buffer_output_flush(buffer);
            pushed = Buffer_try_push(buffer, new);
            assert(pushed);
        }
    }
}
