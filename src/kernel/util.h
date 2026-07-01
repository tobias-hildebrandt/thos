#pragma once

#define align_up(value, align) __builtin_align_up(value, align)
#define is_aligned(value, align) __builtin_is_aligned(value, align)
#define offsetof(type, member) __builtin_offsetof(type, member)

#define STRINGIFY(x) #x

// integer division but rounds up
#define INT_DIV_CEIL(num, denom) ((num + (denom - 1)) / denom)
