#pragma once

#include <stddef.h>

#include "build_info.h"

#if POINTER_BITS == 64
enum { PHYSICAL_PAGE_NUMBER_BITWIDTH = 44 };
#else
enum { PHYSICAL_PAGE_NUMBER_BITWIDTH = 22 };
#endif

enum { PAGE_SIZE = 4096 };

typedef void* Page;

Page Page_alloc(void);
Page Page_alloc_contiguous(size_t size);
