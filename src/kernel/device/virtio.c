#include "device/virtio.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "asm.h"
#include "bits.h"
#include "device/board.h"
#include "panic.h"
#include "virtual_memory/page.h"

// TODO: prevent wrap-around overflows
// TODO: rename block stuff to clarify it is strictly virtio block

enum { VIRTIO_EXPECTED_MAGIC = 0x74726976 };
enum {
    VIRTIO_VERSION_LEGACY = 0x1,
    VIRTIO_VERSION_MODERN = 0x2,
};
enum { VIRTIO_DEVICE_ID_BLOCK = 0x2 };

// register offsets in bytes (not words)
enum VirtioRegisterOffset {
    VIRTIO_REG_MAGIC = 0x000,
    VIRTIO_REG_VERSION = 0x004,
    VIRTIO_REG_DEVICE_ID = 0x008,
    VIRTIO_REG_VENDOR_ID = 0x00c,
    VIRTIO_REG_DEVICE_FEATURES = 0x010,
    VIRTIO_REG_DEVICE_FEATURES_SEL = 0x014,
    VIRTIO_REG_DRIVER_FEATURES = 0x020,
    VIRTIO_REG_DRIVER_FEATURES_SEL = 0x024,
    VIRTIO_REG_QUEUE_SEL = 0x030,
    VIRTIO_REG_QUEUE_SIZE_MAX = 0x034,
    VIRTIO_REG_QUEUE_SIZE = 0x038,
    VIRTIO_REG_QUEUE_READY = 0x044,
    VIRTIO_REG_QUEUE_NOTIFY = 0x050,
    VIRTIO_REG_INTERRUPT_STATUS = 0x060,
    VIRTIO_REG_INTERRUPT_ACK = 0x064,
    VIRTIO_REG_STATUS = 0x070,
    VIRTIO_REG_QUEUE_DESC_LOW = 0x080,
    VIRTIO_REG_QUEUE_DESC_HIGH = 0x084,
    VIRTIO_REG_QUEUE_DRIVER_LOW = 0x90,
    VIRTIO_REG_QUEUE_DRIVER_HIGH = 0x94,
    VIRTIO_REG_QUEUE_DEVICE_LOW = 0xa0,
    VIRTIO_REG_QUEUE_DEVICE_HIGH = 0xa4,
    VIRTIO_REG_SHM_SEL = 0x0ac,
    VIRTIO_REG_SHM_LEN_LOW = 0x0b0,
    VIRTIO_REG_SHM_LEN_HIGH = 0x0b4,
    VIRTIO_REG_SHM_BASE_LOW = 0xb8,
    VIRTIO_REG_SHM_BASE_HIGH = 0xbc,
    VIRTIO_REG_QUEUE_RESET = 0x0c0,
    VIRTIO_REG_CONFIG_GENERATION = 0x0fc,
    VIRTIO_REG_CONFIG = 0x100,
};
typedef enum VirtioRegisterOffset VirtioRegisterOffset;

// status register values
enum {
    VIRTIO_STATUS_ACKNOWLEDGE = 1,
    VIRTIO_STATUS_DRIVER = 2,
    VIRTIO_STATUS_FAILED = 128,
    VIRTIO_STATUS_FEATURES_OK = 8,
    VIRTIO_STATUS_DRIVER_OK = 4,
    VIRTIO_DEVICE_NEEDS_RESET = 64,
};

// feature bits
// TODO: use
enum {
    VIRTIO_FEATURE_SIZE_MAX = 1,
    VIRTIO_FEATURE_SEG_MAX = 2,
    VIRTIO_FEATURE_GEOMETRY = 4,
    VIRTIO_FEATURE_RO = 5,
    VIRTIO_FEATURE_BLK_SIZE = 6,
    VIRTIO_FEATURE_FLUSH = 9,
    VIRTIO_FEATURE_TOPOLOGY = 10,
    VIRTIO_FEATURE_CONFIG_WCE = 11,
    VIRTIO_FEATURE_MQ = 12,
    VIRTIO_FEATURE_DISCARD = 13,
    VIRTIO_FEATURE_WRITE_ZEROES = 14,
    VIRTIO_FEATURE_LIFETIME = 15,
    VIRTIO_FEATURE_SECURE_ERASE = 16,
    VIRTIO_FEATURE_ZONED = 17,
};

struct VirtioDeviceConfig {
    // capacity in 512-byte sectors
    uint64_t capacity;
};
typedef struct VirtioDeviceConfig VirtioDeviceConfig;

// 2.7.5 The Virtqueue Descriptor Table
struct VirtQueueDescriptor {
    /* Address (guest-physical). */
    uint64_t addr;
    /* Length. */
    uint32_t len;

    /* This marks a buffer as continuing via the next field. */
#define VIRTQ_DESC_F_NEXT 1
    /* This marks a buffer as device write-only (otherwise device read-only). */
#define VIRTQ_DESC_F_WRITE 2
    /* This means the buffer contains a list of buffer descriptors. */
#define VIRTQ_DESC_F_INDIRECT 4
    /* The flags as indicated above. */
    uint16_t flags;
    /* Next field if flags & NEXT */
    uint16_t next;
};
typedef struct VirtQueueDescriptor VirtQueueDescriptor;

// 2.7.6 The Virtqueue Available Ring
struct VirtQueueAvailable {
#define VIRTQ_AVAIL_F_NO_INTERRUPT 1
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[/* Queue Size */];
};
typedef struct VirtQueueAvailable VirtQueueAvailable;

// 2.7.8 The Virtqueue Used Ring
/* le32 is used here for ids for padding reasons. */
struct VirtQueueUsedElement {
    /* Index of start of used descriptor chain. */
    uint32_t id;
    /*
     * The number of bytes written into the device writable portion of
     * the buffer described by the descriptor chain.
     */
    uint32_t len;
};
typedef struct VirtQueueUsedElement VirtQueueUsedElement;

// 2.7.8 The Virtqueue Used Ring
struct VirtQueueUsed {
#define VIRTQ_USED_F_NO_NOTIFY 1
    uint16_t flags;
    uint16_t idx;
    VirtQueueUsedElement ring[/* Queue Size */];
};
typedef struct VirtQueueUsed VirtQueueUsed;

enum VirtioRequestType {
    VIRTIO_REQUEST_TYPE_IN = 0,
    VIRTIO_REQUEST_TYPE_OUT = 1,
    VIRTIO_REQUEST_TYPE_FLUSH = 4,
    VIRTIO_REQUEST_TYPE_GET_ID = 8,
    VIRTIO_REQUEST_TYPE_GET_LIFETIME = 10,
    VIRTIO_REQUEST_TYPE_DISCARD = 11,
    VIRTIO_REQUEST_TYPE_WRITE_ZEROES = 13,
    VIRTIO_REQUEST_TYPE_SECURE_ERASE = 14,
};
typedef enum VirtioRequestType VirtioRequestType;

#define VIRTIO_REQUEST_STATUS_OK 0
#define VIRTIO_REQUEST_STATUS_IOERR 1
#define VIRTIO_REQUEST_STATUS_UNSUPP 2
typedef char VirtioRequestStatus;

// 5.2.6 Device Operation
struct VirtioRequestHeader {
    VirtioRequestType type;
    uint32_t reserved;
    uint64_t sector;
    // does not include
    // uint8_t data[];
    uint8_t status;
} __attribute__((packed));
typedef struct VirtioRequestHeader VirtioRequestHeader;

#define PRINT_FEATURE_ENABLED(FEATURE_VALUE, FEATURE_BIT)    \
    printf("feature %s: %u\n",                               \
           ((#FEATURE_BIT) + sizeof("VIRTIO_FEATURE_") - 1), \
           BIT_GET(FEATURE_VALUE, FEATURE_BIT))

typedef uint32_t VirtioRegisterValue;
typedef uint16_t VirtioDescriptorIndex;

static VirtioRegisterValue VirtioRegister_read(const void* base,
                                               VirtioRegisterOffset offset) {
    VirtioRegisterValue out;
    ASM("ld %[out], (%[addr])\n" : [out] "=r"(out) : [addr] "r"((char*)base +
                                                                offset));
    return out;
}

static void VirtioRegister_write(const void* base, VirtioRegisterOffset offset,
                                 VirtioRegisterValue value) {
    ASM("sw %[value], (%[addr])\n"
        //
        ::[value] "r"(value),
        [addr] "r"(((char*)base) + offset));
}

static void VirtioRegister_write_or(const void* base,
                                    VirtioRegisterOffset offset,
                                    VirtioRegisterValue value) {
    VirtioRegister_write(base, offset,
                         VirtioRegister_read(base, offset) | value);
}

// TODO: move to generated flags.h
#define VIRTIO_QUEUE_SIZE 128

// TODO: async read/write
struct VirtioDevice {
    // MUST access registers through accessors
    VirtioRegisterValue* base;

    // "buffer"s
    VirtQueueDescriptor* descriptors;
    VirtQueueAvailable* available;
    VirtQueueUsed* used;

    // TODO: mapping of descriptor chain id -> callback?
    // TODO: or return a token that can be used to check used

    VirtioDescriptorIndex last_written_descriptor;
    VirtioDescriptorIndex old_used_index;
};
typedef struct VirtioDevice VirtioDevice;

// TODO: support multiple devices, use (sleep)locks
static VirtioDevice virtio_state;
static VirtioDevice* VirtioDevice_try_from_base(void* base) {
    VirtioRegisterValue magic = VirtioRegister_read(base, VIRTIO_REG_MAGIC);
    if (magic != VIRTIO_EXPECTED_MAGIC) {
        return NULL;
    }
    VirtioRegisterValue version = VirtioRegister_read(base, VIRTIO_REG_VERSION);
    printf("virtio version: %u\n", version);

    VirtioRegisterValue device_id =
        VirtioRegister_read(base, VIRTIO_REG_DEVICE_ID);
    printf("virtio device id: 0x%08x\n", device_id);

    // TODO: allow/use all valid devices instead of only first
    if (virtio_state.base == NULL && version == VIRTIO_VERSION_MODERN &&
        device_id == VIRTIO_DEVICE_ID_BLOCK) {
        virtio_state.base = base;

        return &virtio_state;
    } else {
        return NULL;
    }
}

static void VirtioDevice_set_features(VirtioDevice* device) {
    // select features to read
    VirtioRegister_write(device->base, VIRTIO_REG_DEVICE_FEATURES_SEL, 0);

    // read features
    VirtioRegisterValue device_features =
        VirtioRegister_read(device->base, VIRTIO_REG_DEVICE_FEATURES);

    // print features
    printf("device features: 0b%032b\n", device_features);
    PRINT_FEATURE_ENABLED(device_features, VIRTIO_FEATURE_SIZE_MAX);
    PRINT_FEATURE_ENABLED(device_features, VIRTIO_FEATURE_SEG_MAX);
    PRINT_FEATURE_ENABLED(device_features, VIRTIO_FEATURE_GEOMETRY);
    PRINT_FEATURE_ENABLED(device_features, VIRTIO_FEATURE_RO);
    PRINT_FEATURE_ENABLED(device_features, VIRTIO_FEATURE_BLK_SIZE);
    PRINT_FEATURE_ENABLED(device_features, VIRTIO_FEATURE_FLUSH);
    PRINT_FEATURE_ENABLED(device_features, VIRTIO_FEATURE_TOPOLOGY);
    PRINT_FEATURE_ENABLED(device_features, VIRTIO_FEATURE_CONFIG_WCE);
    PRINT_FEATURE_ENABLED(device_features, VIRTIO_FEATURE_MQ);
    PRINT_FEATURE_ENABLED(device_features, VIRTIO_FEATURE_DISCARD);
    PRINT_FEATURE_ENABLED(device_features, VIRTIO_FEATURE_WRITE_ZEROES);
    PRINT_FEATURE_ENABLED(device_features, VIRTIO_FEATURE_LIFETIME);
    PRINT_FEATURE_ENABLED(device_features, VIRTIO_FEATURE_SECURE_ERASE);
    PRINT_FEATURE_ENABLED(device_features, VIRTIO_FEATURE_ZONED);

    // TODO: actually support some useful fatures
    VirtioRegisterValue our_supported_features = 0;

    // select features to write
    VirtioRegister_write(device->base, VIRTIO_REG_DRIVER_FEATURES_SEL, 0);
    // write features
    VirtioRegister_write(device->base, VIRTIO_REG_DRIVER_FEATURES,
                         our_supported_features);

    // tell device done writing supported features
    VirtioRegister_write_or(device->base, VIRTIO_REG_STATUS,
                            VIRTIO_STATUS_FEATURES_OK);

    VirtioRegisterValue status =
        VirtioRegister_read(device->base, VIRTIO_REG_STATUS);
    printf("status after feat:  0b%08b\n", status);

    if ((status & VIRTIO_STATUS_FEATURES_OK) != VIRTIO_STATUS_FEATURES_OK) {
        PANIC("device does not support our features");
    }
}

static void VirtioDevice_init_queues(VirtioDevice* device) {
    // initialize virt queues
    VirtioRegister_write(device->base, VIRTIO_REG_QUEUE_SEL, 0);
    VirtioRegisterValue queue_ready =
        VirtioRegister_read(device->base, VIRTIO_REG_QUEUE_READY);

    // expect queue ready to be zero
    if (queue_ready != 0) {
        PANIC("queue 0 not ready?");
    }

    // determine queue size
    // TODO: actually use max queue size

    VirtioRegisterValue queue_size_max =
        VirtioRegister_read(device->base, VIRTIO_REG_QUEUE_SIZE_MAX);

    printf("max queue size: %d\n", queue_size_max);

    uint32_t queue_size = VIRTIO_QUEUE_SIZE;
    printf("our queue size: %d\n", queue_size);

    // write queue size
    VirtioRegister_write(device->base, VIRTIO_REG_QUEUE_SIZE, queue_size);
    fence();

    // TODO: bundle into struct, allocate all in one big contiguous block

    // array of queue_size descriptors
    VirtQueueDescriptor* descriptors =
        (VirtQueueDescriptor*)Page_alloc_contiguous(16 * (size_t)queue_size);
    // driver request queue
    VirtQueueAvailable* available = Page_alloc_contiguous(6 + (2 * queue_size));
    // device used queue
    VirtQueueUsed* used = Page_alloc_contiguous(6 + (8 * queue_size));

    // store
    device->descriptors = descriptors;
    device->available = available;
    device->used = used;
    device->last_written_descriptor = VIRTIO_QUEUE_SIZE - 1;
    device->old_used_index = VIRTIO_QUEUE_SIZE - 1;

    printf("descriptor:     %p\n", descriptors);
    printf("available:      %p\n", available);
    printf("used            %p\n", used);

    // write to registers

    // descriptor = descriptor area
    VirtioRegisterValue desc_low = (uintptr_t)descriptors;
    VirtioRegisterValue desc_high = ((uintptr_t)descriptors) >> 32;
    printf("desc low        0x%08x\n", desc_low);
    printf("desc high       0x%08x\n", desc_high);
    VirtioRegister_write(device->base, VIRTIO_REG_QUEUE_DESC_LOW, desc_low);
    VirtioRegister_write(device->base, VIRTIO_REG_QUEUE_DESC_HIGH, desc_high);

    // available = driver area
    VirtioRegisterValue driver_low = (uintptr_t)available;
    VirtioRegisterValue driver_high = ((uintptr_t)available) >> 32;
    printf("driver low      0x%08x\n", driver_low);
    printf("driver high     0x%08x\n", driver_high);
    VirtioRegister_write(device->base, VIRTIO_REG_QUEUE_DRIVER_LOW, driver_low);
    VirtioRegister_write(device->base, VIRTIO_REG_QUEUE_DRIVER_HIGH,
                         driver_high);

    // used = device area
    VirtioRegisterValue device_low = (uintptr_t)used;
    VirtioRegisterValue device_high = ((uintptr_t)used) >> 32;
    printf("device low      0x%08x\n", device_low);
    printf("device high     0x%08x\n", device_high);
    VirtioRegister_write(device->base, VIRTIO_REG_QUEUE_DEVICE_LOW, device_low);
    VirtioRegister_write(device->base, VIRTIO_REG_QUEUE_DEVICE_HIGH,
                         device_high);

    fence();

    // write queue ready
    VirtioRegister_write(device->base, VIRTIO_REG_QUEUE_READY, 0x1);
    fence();
}

// increment and loop an index, then return the new value
static VirtioDescriptorIndex VirtioDescriptorIndex_increment_and_get(
    VirtioDescriptorIndex* start) {
    *start = (*start + 1) % VIRTIO_QUEUE_SIZE;
    return *start;
}

#define VIRTIO_REQUEST_SIZE(DATA_SIZE) \
    (sizeof(VirtioRequestHeader) + (data_size) + 1)
#define VIRTIO_REQUEST_DECLARE char request[VIRTIO_REQUEST_SIZE(data_size)]

// submits request to virtio.
// caller must set request type in header.
// caller must guarantee lifetime of header and data.
static VirtioRequestStatus VirtioDevice_request(VirtioDevice* device,
                                                VirtioRequestHeader* header,
                                                char* data, size_t data_size,
                                                bool data_write) {
    char* status = (char*)&header->status;

    printf("header data address         %p\n", header);
    printf("data   data address         %p\n", data);
    printf("status data address         %p\n", status);

    // descriptor IDs
    VirtioDescriptorIndex desc_index_header =
        VirtioDescriptorIndex_increment_and_get(
            &device->last_written_descriptor);
    VirtioDescriptorIndex desc_index_data =
        VirtioDescriptorIndex_increment_and_get(
            &device->last_written_descriptor);
    VirtioDescriptorIndex desc_index_status =
        VirtioDescriptorIndex_increment_and_get(
            &device->last_written_descriptor);

    // header descriptor
    VirtQueueDescriptor* desc_header = &device->descriptors[desc_index_header];
    printf("header descriptor address   %p\n", desc_header);
    desc_header->addr = (uint64_t)(uintptr_t)header;
    desc_header->len = sizeof(VirtioRequestHeader) - 1;  // don't include status
    // read-only and chained
    desc_header->flags = VIRTQ_DESC_F_NEXT;
    desc_header->next = desc_index_data;

    // data descriptor
    VirtQueueDescriptor* desc_data = &device->descriptors[desc_index_data];
    printf("data   descriptor address   %p\n", desc_data);
    desc_data->addr = (uint64_t)(uintptr_t)data;
    desc_data->len = data_size;
    // write-only and chained
    desc_data->flags =
        VIRTQ_DESC_F_NEXT | (data_write ? VIRTQ_DESC_F_WRITE : 0);
    desc_data->next = desc_index_status;

    // status descriptor
    VirtQueueDescriptor* desc_status = &device->descriptors[desc_index_status];
    printf("status descriptor address   %p\n", desc_status);
    desc_status->addr = (uint64_t)(uintptr_t)status;
    desc_status->len = 1;
    // write-only and end of chain
    desc_status->flags = VIRTQ_DESC_F_WRITE;

    fence();

    // push chain into ring
    VirtioDescriptorIndex chain_id = desc_index_header;
    device->available->ring[device->available->idx] = chain_id;
    fence();
    device->available->idx += 1;
    fence();

    // notify device that queue has available buffer
    VirtioRegister_write(device->base, VIRTIO_REG_QUEUE_NOTIFY, 0x0);
    fence();

    // TODO: enable plic and handle interrupt asynchronously through trap
    VirtioRegisterValue interrupt;
    do {
        fence();
        interrupt =
            VirtioRegister_read(device->base, VIRTIO_REG_INTERRUPT_STATUS);
    } while (interrupt == 0);

    printf("VIRTIO INTERRUPT\n");
    printf("interrupt: 0b%02b\n", interrupt);
    printf("device->old_used_index: %u\n", device->old_used_index);
    printf("device->used->idx:      %u\n", device->used->idx);

    // TODO: use mapping
    VirtioDescriptorIndex used_index =
        VirtioDescriptorIndex_increment_and_get(&device->old_used_index);
    VirtQueueUsedElement* used_elem = &device->used->ring[used_index];
    printf("device used descriptor chain %u, wrote %u bytes\n", used_elem->id,
           used_elem->len);

    printf("status: %u\n", *status);

    // ACK the interrupt
    VirtioRegister_write(device->base, VIRTIO_REG_INTERRUPT_ACK, interrupt);

    return *status;
}

void virtio_init(void) {
    VirtioDevice* device = NULL;

    // TODO: try all from device tree
    for (int device_number = 0; device_number < 8; device_number++) {
        void* device_ptr = GET_BOARD_DEVICE(board.virtio[device_number]);
        printf("virtio device #%u @ %p\n", device_number, device_ptr);

        VirtioDevice* this = VirtioDevice_try_from_base(device_ptr);
        if (this != NULL) {
            device = this;
            break;
        }
    }

    if (device == NULL) {
        PANIC("could not find valid virtio device id");
    }

    VirtioRegisterValue status =
        VirtioRegister_read(device->base, VIRTIO_REG_STATUS);
    printf("status initial:     0b%08b\n", status);

    // reset
    VirtioRegister_write(device->base, VIRTIO_REG_STATUS, 0);

    // acknowledge that there is a device
    VirtioRegister_write_or(device->base, VIRTIO_REG_STATUS,
                            VIRTIO_STATUS_ACKNOWLEDGE);

    // tell the device that we are a driver
    VirtioRegister_write_or(device->base, VIRTIO_REG_STATUS,
                            VIRTIO_STATUS_DRIVER);

    status = VirtioRegister_read(device->base, VIRTIO_REG_STATUS);
    printf("status first:       0b%08b\n", status);

    // set features
    VirtioDevice_set_features(device);

    // initialize virt queues
    VirtioDevice_init_queues(device);

    // TODO: activate interrupt on plic

    // TODO: actually make sure we don't surpass this
    // TODO: actually read a u64
    uint64_t capacity = VirtioRegister_read(device->base, VIRTIO_REG_CONFIG);
    printf("max capacity: %u sectors (%u bytes)\n", capacity, capacity * 512);

    // driver is READY
    VirtioRegister_write_or(device->base, VIRTIO_REG_STATUS,
                            VIRTIO_STATUS_DRIVER_OK);
    fence();

    // get ID
    {
        char device_id[21];
        VirtioRequestHeader header = {.type = VIRTIO_REQUEST_TYPE_GET_ID};
        for (int i = 0; i <= 3; i++) {
            memset(device_id, 0, 21);

            if (VIRTIO_REQUEST_STATUS_OK ==
                VirtioDevice_request(device, &header, device_id, 20, true)) {
                printf("device id: '%s'\n", device_id);
            } else {
                printf("device id request failed\n");
            }
        }
    }

    // do some real IO
    {
        const size_t data_size = VIRTIO_BLOCK_SIZE;
        char data[data_size];
        VirtioRequestHeader header = {.type = VIRTIO_REQUEST_TYPE_OUT};
        for (int i = 0; i < 5; i++) {
            for (size_t i = 0; i < data_size; i++) {
                data[i] = i % data_size;
            }
            header.sector = i;

            if (VIRTIO_REQUEST_STATUS_OK ==
                VirtioDevice_request(device, &header, data, data_size, false)) {
                printf("write data to sector %u OK\n", i);
            } else {
                printf("write data to sector %u failed\n", i);
            }
        }

        memset(data, 0, data_size);

        header.type = VIRTIO_REQUEST_TYPE_IN;
        for (int i = 0; i < 5; i++) {
            header.sector = i;

            if (VIRTIO_REQUEST_STATUS_OK ==
                VirtioDevice_request(device, &header, data, data_size, true)) {
                printf("read data from sector %u OK\n", i);
                for (size_t i = 0; i < data_size; i++) {
                    if (i % 16 == 0 && i != 0) {
                        printf("\n");
                    }

                    printf("%02x ", data[i]);
                }
                printf("\n");

            } else {
                printf("read data from sector %u failed\n", i);
            }
        }
    }
}
