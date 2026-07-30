#pragma once

#include <stdint.h>  // IWYU pragma: keep, uintptr_t

#include "build_info.h"

#if COMPILER_IS_CLANG && !defined(__FRAMAC__)
#define align_up(value, align) __builtin_align_up(value, align)
#define is_aligned(value, align) __builtin_is_aligned(value, align)
#define align_up_ptr(ptr, align) __builtin_align_up(ptr, align)
#else
#define align_up(value, align) \
    ((((value) % align) == 0) ? (value) : ((value) + align - ((value) % align)))
#define is_aligned(value, align) (0 == value % align)
#define align_up_ptr(ptr, align) ((void*)align_up((uintptr_t)ptr, align))
#endif
