#pragma once

#include <stdint.h>

#include "device/device_tree.h"

void kernel_main(uintptr_t hart_id,
                 const DeviceTreeHeadersRaw* device_tree_headers);
