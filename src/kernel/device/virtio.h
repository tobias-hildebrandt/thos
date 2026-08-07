#pragma once

// TODO: parse from device tree

#define VIRTIO_0_ADDRESS 0x10001000UL
#define VIRTIO_BLOCK_SIZE 512

#define VIRTIO_ADDRESS_X(x) \
    ((void*)((char*)VIRTIO_0_ADDRESS + (uintptr_t)((x) * 0x1000)))
#define VIRTIO_ARRAY                                                   \
    {                                                                  \
        VIRTIO_ADDRESS_X(7), VIRTIO_ADDRESS_X(6), VIRTIO_ADDRESS_X(5), \
        VIRTIO_ADDRESS_X(4), VIRTIO_ADDRESS_X(3), VIRTIO_ADDRESS_X(2), \
        VIRTIO_ADDRESS_X(1), VIRTIO_ADDRESS_X(0),                      \
    }

void virtio_init(void);
