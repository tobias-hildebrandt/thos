#pragma once

// RISC-V ABIs Specification Chapter 4, LP64

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;

typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long int64_t;

typedef uint64_t uintptr_t;
typedef int64_t intptr_t;

#define INT8_MIN -128
#define INT8_MAX 127
#define UINT8_MAX 255

#define INT16_MIN -32768
#define INT16_MAX 32767
#define UINT16_MAX 65535

// two's complement
#define INT32_MAX 0x7fffffff
#define INT32_MIN 0x80000000
#define UINT32_MAX 0xffffffff

#define INT64_MAX ((uint64_t)0x7fffffffffffffff)
#define INT64_MIN ((uint64_t)0x8000000000000000)
#define UINT64_MAX ((uint64_t)0xffffffffffffffff)
