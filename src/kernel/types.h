#pragma once

#define align_up(value, align) __builtin_align_up(value, align)
#define is_aligned(value, align) __builtin_is_aligned(value, align)
#define offsetof(type, member) __builtin_offsetof(type, member)

#define REGISTER(REGISTER) register long REGISTER __asm__(#REGISTER)

#define STRINGIFY(x) #x
