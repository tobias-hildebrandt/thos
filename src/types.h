#pragma once

// TODO https://en.cppreference.com/c/types

// 64 bit risc-v
typedef unsigned char uint8_t;
typedef char int8_t;
typedef unsigned int uint32_t;
typedef int int32_t;
typedef unsigned long uint64_t;
typedef long int64_t;

typedef uint64_t uintptr_t;
typedef uint64_t size_t;

// c99 bool
typedef _Bool bool;
#define true 1
#define false 0

#define va_list __builtin_va_list
#define va_start __builtin_va_start
#define va_end __builtin_va_end
#define va_arg __builtin_va_arg

#define align_up(value, align) __builtin_align_up(value, align)
#define is_aligned(value, align) __builtin_is_aligned(value, align)
#define offsetof(type, member) __builtin_offsetof(type, member)

#define NULL ((void*)0)

#define REGISTER(REGISTER) register long REGISTER __asm__(#REGISTER)

// two's complement
#define INT_MAX 0x7fffffff
#define INT_MIN 0x80000000

#define STRINGIFY(x) #x
