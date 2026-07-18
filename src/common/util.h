#pragma once

// stringify a token
#define STRINGIFY(x) #x
#define STRINGIFY_VALUE(x) STRINGIFY(x)
// concat two tokens with an underscore
#define CONCAT_(a, b) a##_##b
#define CONCAT3_(a, b, c) a##_##b##_##c

// integer division but rounds up
#define INT_DIV_CEIL(num, denom) (((num) + ((denom) - 1)) / (denom))

#define LOGICAL_XOR(cond1, cond2) (!(cond1) != !(cond2))

#define NAKED __attribute__((naked))
#define UNUSED __attribute__((unused))
#define SECTION(s) __attribute((section(s)))

// number of non-nul characters in a string literal
#define CHARS_IN_STRING_LITERAL(x) (sizeof((x)) - 1)
