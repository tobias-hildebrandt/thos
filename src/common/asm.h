#pragma once

#include <stddef.h>
#include <stdint.h>

#include "build_info.h"

// inline assembly
#define ASM(...) __asm__ __volatile__(__VA_ARGS__)

#define ASM_SET_LABEL(x) x ":\n"

// define ASM string for register/address<->mem operations
#if POINTER_BITS == 64
#define ASM_LOAD "ld "
#define ASM_STORE "sd "
#else
#define ASM_LOAD "lw "
#define ASM_STORE "sw "
#endif

// asm string generation to load/store registers in memory
// NOLINTBEGIN(bugprone-macro-parentheses)
#define REGISTER_MEM(instr, base, reg, type) \
    ASM(instr #reg ", %[offset](" #base      \
                   ")\n" : : [offset] "i"(offsetof(type, reg)))
// NOLINTEND(bugprone-macro-parentheses)

// TODO: determine if register macros are even useful

// declare a register variable
#define NAMED_REGISTER(NAME, REGISTER) register long(NAME) __asm__(#REGISTER)
#define REGISTER(REGISTER) NAMED_REGISTER((REGISTER), (REGISTER))
#define REGISTER_INIT(REGISTER, init)   \
    NAMED_REGISTER(REGISTER, REGISTER); \
    (REGISTER) = init;

#if COMPILER_IS_CLANG
// clang-format off
#define REGS_START                   \
    _Pragma("clang diagnostic push") \
    _Pragma("clang diagnostic ignored \"-Wuninitialized\"")
#define REGS_END _Pragma("clang diagnostic pop")
// clang-format on
#else
#define REGS_START
#define REGS_END
#endif

// atomically OR a word in memory
static inline uint32_t atomic_or_memory_word(const uint32_t* ptr,
                                             uint32_t value) {
    uint32_t out;
    ASM("amoor.w %[out], %[source], (%[store])"
        //
        : [out] "=r"(out)
        //
        : [source] "r"(value),
        [store] "r"(ptr)
        //
        : "memory");
    return out;
}
