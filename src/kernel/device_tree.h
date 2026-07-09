#pragma once

#include <stddef.h>
#include <stdint.h>

#include "endian.h"

#define DEVICE_TREE_MAGIC BIG_TO_LITTLE32(0xD00DFEED)

#define DEVICE_TREE_ROOT_PATH ""

// 5.2 Header
// Flattened Devicetree Header Fields
// All the header fields are 32-bit integers, stored in big - endian format.
struct DeviceTreeHeadersRaw {
    uint32_t magic;
    uint32_t totalsize;
    uint32_t off_dt_struct;
    uint32_t off_dt_strings;
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;
    uint32_t size_dt_struct;
};
typedef struct DeviceTreeHeadersRaw DeviceTreeHeadersRaw;

// parsed device tree node property
typedef struct DeviceTreeProperty DeviceTreeProperty;
struct DeviceTreeProperty {
    const char* name;
    const void* value;
    size_t value_len;
    // singly linked list sibling
    DeviceTreeProperty* next;
};

// parsed device tree node
typedef struct DeviceTreeNode DeviceTreeNode;
struct DeviceTreeNode {
    const char* name;
    DeviceTreeProperty* properties;
    DeviceTreeNode* child;
    // singly linked list sibling
    DeviceTreeNode* next;
};

struct DeviceTree {
    uint32_t boot_cpuid_phys;
    DeviceTreeNode* root;
};
typedef struct DeviceTree DeviceTree;

DeviceTree DeviceTree_parse(const DeviceTreeHeadersRaw* header);
void DeviceTree_print(DeviceTree* tree);
DeviceTreeNode* DeviceTreeNode_find_child(DeviceTreeNode* node, char** path,
                                          uint8_t offset);
DeviceTreeProperty* DeviceTreeNode_find_property(DeviceTreeNode* node,
                                                 char* property_name);
void DeviceTree_dump_raw(const DeviceTreeHeadersRaw* header);
