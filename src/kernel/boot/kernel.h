#pragma once

#include <stdint.h>

#include "device/device_tree.h"
#include "util.h"

void NORETURN kernel_main(uintptr_t hart_id,
                          const DeviceTreeHeadersRaw* device_tree_headers);
