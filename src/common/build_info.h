#pragma once

#include "util.h"  // IWYU pragma: keep

#if defined(__clang__)
#define COMPILER_IS_CLANG 1
#define COMPILER_VERSION __clang_version__
#define COMPILER_STRING "clang " COMPILER_VERSION
#elif defined(__GNUC__)
#define COMPILER_IS_GNU 1
#define COMPILER_VERSION      \
    STRINGIFY_VALUE(__GNUC__) \
    "." STRINGIFY_VALUE(__GNUC_MINOR__) "." STRINGIFY_VALUE(__GNUC_PATCHLEVEL__)
#define COMPILER_STRING "gcc " COMPILER_VERSION
#else
#pragma message "unsupported compiler?"
#define COMPILER_VERSION "unknown"
#define COMPILER_STRING "unknown"
#endif

#if __SIZEOF_POINTER__ == 8
// NOLINTNEXTLINE(modernize-macro-to-enum)
#define POINTER_BITS 64
#define ONLY64(expr) expr
#define ONLY32(expr)
#elif __SIZEOF_POINTER__ == 4
// NOLINTNEXTLINE(modernize-macro-to-enum)
#define POINTER_BITS 32
#define ONLY64(expr)
#define ONLY32(expr) expr
#else
#error Unsupported platform
#endif
