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

#define va_list __builtin_va_list
#define va_start __builtin_va_start
#define va_end __builtin_va_end
#define va_arg __builtin_va_arg

#define NULL ((void*)0)

#define REGISTER(REGISTER) register long REGISTER __asm__(#REGISTER)
