#include "list.h"

#include <stddef.h>
#include <stdio.h>

#include "flags.h"
#include "io.h"

void add_linked(void** base, void* next, size_t next_ptr_offset) {
    // TODO: avoid loop-arounds, keep track of base
    void** insert_at = base;
    while (*insert_at != NULL) {
        insert_at = (void**)((*(char**)insert_at) + next_ptr_offset);
    }
    PRINTF_IF(DEBUG_LIST, "list.h adding to linkedlist\n");

    *insert_at = next;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void* next_free(void* array, size_t* next, size_t size, size_t max) {
    if (*next >= max - 1) {
        printf("list.h ran out of items\n");
        return NULL;
    }
    void* pointer = (void*)((char*)(array) + (size * *next));
    *next += 1;

    PRINTF_IF(DEBUG_LIST, "list.h allocated new. next is now %d\n", *next);

    return pointer;
}
