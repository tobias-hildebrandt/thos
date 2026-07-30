#pragma once

#include <stdint.h>

#include "util.h"

void NORETURN real_boot(uintptr_t hart_id, uintptr_t device_tree);
