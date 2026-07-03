#pragma once

#include <stddef.h>

// inline assembly
#define ASM(...) __asm__ __volatile__(__VA_ARGS__)

#define ASM_SET_LABEL(x) x ":\n"

// asm string generation to load/store registers in memory
#define REGISTER_MEM(instr, base, reg, type) \
    ASM(#instr " " #reg ", %[offset](" #base \
               ")\n" : : [offset] "i"(offsetof(type, reg)))

// declare a register variable
#define NAMED_REGISTER(NAME, REGISTER) register long NAME __asm__(#REGISTER)
#define REGISTER(REGISTER) NAMED_REGISTER(REGISTER, REGISTER)
