#pragma once

#include <stddef.h>
#include <stdio.h>

#include "util.h"

// allocate NUM items as a static array, along with a counter
#define ALLOCATE_ARRAY_AND_COUNTER(TAG, TYPE, NUM) \
    static size_t CONCAT_(TAG, next) = 0;          \
    static TYPE CONCAT_(TAG, items)[NUM]

// claim next free item in static array
#define NEXT_FREE(TAG, TYPE, MAX) \
    next_free(CONCAT_(TAG, items), &CONCAT_(TAG, next), sizeof(TYPE), (MAX))

// add an item to the end of a singly linked list
#define ADD_LINKED(OLD, NEW, TYPE, NEXT_PTR) \
    add_linked((void**)&(OLD), (void*)(NEW), offsetof(TYPE, NEXT_PTR))

// use ADD_LINKED
void add_linked(void** base, void* next, size_t next_ptr_offset);

// use NEXT_FREE
void* next_free(void* array, size_t* next, size_t size, size_t max);
