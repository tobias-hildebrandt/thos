#pragma once

#include "build_info.h"

#if COMPILER_IS_CLANG
#define align_up(value, align) __builtin_align_up(value, align)
#define is_aligned(value, align) __builtin_is_aligned(value, align)
#else
#define align_up(value, align) \
    ((((value) % align) == 0) ? (value) : ((value) + align - ((value) % align)))
#define is_aligned(value, align) (0 == value % align)
#endif
