#pragma once

#include <stdbool.h>
#include <stdint.h>

struct SpinLock {
    uint32_t locked;
    uintptr_t hart_owner;
};
typedef struct SpinLock SpinLock;

bool SpinLock_acquire(SpinLock* lock);
void SpinLock_release(SpinLock* lock);
