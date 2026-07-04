#pragma once

#define align_up(value, align) __builtin_align_up(value, align)
#define is_aligned(value, align) __builtin_is_aligned(value, align)

// stringify a token
#define STRINGIFY(x) #x
// concat two tokens with an underscore
#define CONCAT_(a, b) a##_##b

// integer division but rounds up
#define INT_DIV_CEIL(num, denom) ((num + (denom - 1)) / denom)

#define LOGICAL_XOR(cond1, cond2) \
    (((cond1) && (!cond2)) || ((cond2) && (!cond1)))
