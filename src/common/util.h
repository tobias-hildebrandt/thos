#pragma once

// stringify a token
#define STRINGIFY(x) #x
#define STRINGIFY_VALUE(x) STRINGIFY(x)
// concat two tokens with an underscore
#define CONCAT_(a, b) a##_##b
#define CONCAT3_(a, b, c) a##_##b##_##c

// integer division but rounds up
#define INT_DIV_CEIL(num, denom) ((num + (denom - 1)) / denom)

#define LOGICAL_XOR(cond1, cond2) (!(cond1) != !(cond2))

#include "build_info.h"

#if COMPILER_IS_CLANG
#define align_up(value, align) __builtin_align_up(value, align)
#define is_aligned(value, align) __builtin_is_aligned(value, align)
#else
#define align_up(value, align) \
    ((((value) % align) == 0) ? (value) : ((value) + align - ((value) % align)))
#define is_aligned(value, align) (0 == value % align)
#endif

#define NAKED __attribute__((naked))
#define UNUSED __attribute__((unused))
