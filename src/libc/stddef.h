#pragma once

#include <stdint.h>

#define NULL ((void*)0)

typedef uint64_t size_t;

#define offsetof(type, member) __builtin_offsetof(type, member)
