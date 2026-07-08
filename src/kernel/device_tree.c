#include "device_tree.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "flags.h"
#include "io.h"
#include "panic.h"
#include "util.h"

#define DEVICE_TREE_VERSION BIG_TO_LITTLE32(17)

enum DeviceTreeStructureBlockToken {
    STRUCTURE_BEGIN_NODE = BIG_TO_LITTLE32(0x01),
    STRUCTURE_END_NODE = BIG_TO_LITTLE32(0x02),
    STRUCTURE_PROP = BIG_TO_LITTLE32(0x03),
    STRUCTURE_NOP = BIG_TO_LITTLE32(0x04),
    STRUCTURE_END = BIG_TO_LITTLE32(0x09),
};
typedef enum DeviceTreeStructureBlockToken DeviceTreeStructureBlockToken;

struct DeviceTreeStructurePropertyRaw {
    uint32_t len;
    uint32_t nameoff;
};
typedef struct DeviceTreeStructurePropertyRaw DeviceTreeStructurePropertyRaw;

// TODO: macro these singly-linked lists, pre-alloc, limits, etc

#define NUM_DEVICE_TREE_PROPERTIES 1024
#define NUM_DEVICE_TREE_NODES 256

static size_t next_node_index = 0;
static size_t next_property_index = 0;

static DeviceTreeNode nodes[NUM_DEVICE_TREE_NODES];
static DeviceTreeProperty properties[NUM_DEVICE_TREE_PROPERTIES];

void add_linked_prop(DeviceTreeProperty** base, DeviceTreeProperty* next) {
    DeviceTreeProperty** insert_at = base;
    while (*insert_at != NULL) {
        insert_at = &(*insert_at)->next;
    }
    PRINTF_IF(DEBUG_DEVICE_TREE, "set prop\n");

    *insert_at = next;
}

void add_linked_node(DeviceTreeNode** base, DeviceTreeNode* next) {
    DeviceTreeNode** insert_at = base;
    while (*insert_at != NULL) {
        insert_at = &(*insert_at)->next;
    }
    PRINTF_IF(DEBUG_DEVICE_TREE, "set node\n");
    *insert_at = next;
}

DeviceTreeNode* next_free_node(void) {
    if (next_node_index >= NUM_DEVICE_TREE_NODES - 1) {
        PANIC("ran out of free device tree nodes");
    }

    DeviceTreeNode* new = &nodes[next_node_index];
    next_node_index += 1;
    return new;
}

DeviceTreeProperty* next_free_property(void) {
    if (next_property_index >= NUM_DEVICE_TREE_PROPERTIES - 1) {
        PANIC("ran out of free device tree properties");
    }

    DeviceTreeProperty* new = &properties[next_property_index];
    next_property_index += 1;
    return new;
}

void skip_nops(const uint32_t** pointer) {
    while (**pointer == STRUCTURE_NOP) {
        PRINTF_IF(DEBUG_DEVICE_TREE, "skipping NOP\n");
        *pointer += 1;
    }
}

char* parse_node_name(char** pointer) {
    char* node_name = *pointer;
    char* node_name_end = *pointer;
    while (*node_name_end != 0) {
        node_name_end += 1;
    }
    node_name_end += 1;
    PRINTF_IF(DEBUG_DEVICE_TREE, "node (strlen=%d): \"%s\"\n",
              node_name_end - node_name, node_name);

    // move pointer up to end and align it
    *pointer = (void*)align_up((uintptr_t)node_name_end, 4);
    PRINTF_IF(DEBUG_DEVICE_TREE, "after node name: %p\n", *pointer);

    return node_name;
}

DeviceTreeProperty* parse_prop(const DeviceTreeHeadersRaw* header,
                               const uint32_t** pointer) {
    skip_nops(pointer);

    if (**pointer != STRUCTURE_PROP) {
        return NULL;
    }
    PRINTF_IF(DEBUG_DEVICE_TREE, "PROP @ %p, offset = %#x\n", *pointer,
              (uintptr_t)*pointer - (uintptr_t)header);

    *pointer += 1;

    DeviceTreeProperty* parsed = next_free_property();

    DeviceTreeStructurePropertyRaw* property_raw =
        (DeviceTreeStructurePropertyRaw*)*pointer;

    // move past raw property
    *pointer =
        (void*)(((uintptr_t)*pointer) + sizeof(DeviceTreeStructurePropertyRaw));

    // grab name via header offset
    parsed->name =
        (char*)((uintptr_t)header + BIG_TO_LITTLE32(header->off_dt_strings) +
                BIG_TO_LITTLE32(property_raw->nameoff));

    PRINTF_IF(DEBUG_DEVICE_TREE, "prop name: \"%s\"\n", parsed->name);
    PRINTF_IF(DEBUG_DEVICE_TREE, "prop name nameoff: %d\n",
              BIG_TO_LITTLE32(property_raw->nameoff));

    // store pointer and length
    parsed->value = (void*)*pointer;
    parsed->value_len = BIG_TO_LITTLE32(property_raw->len);

    PRINTF_IF(DEBUG_DEVICE_TREE, "prop len: %d\n", parsed->value_len);

    if (DEBUG_DEVICE_TREE) {
        printf("prop data: ");
        for (size_t i = 0; i < parsed->value_len; i++) {
            printf("%02x ", ((char*)parsed->value)[i]);
        }
        printf("\n");
    }

    // move past property data
    *pointer = (void*)((uintptr_t)*pointer + parsed->value_len);

    // align
    *pointer = (void*)(align_up((uintptr_t)*pointer, 4));

    PRINTF_IF(DEBUG_DEVICE_TREE, "post-prop pointer: %p\n", *pointer);

    return parsed;
}

DeviceTreeNode* parse_node(const DeviceTreeHeadersRaw* header,
                           const uint32_t** pointer) {
    skip_nops(pointer);

    if (**pointer != STRUCTURE_BEGIN_NODE) {
        return NULL;
    }
    PRINTF_IF(DEBUG_DEVICE_TREE, "BEGIN_NODE\n");
    *pointer += 1;

    DeviceTreeNode* parsed = next_free_node();

    parsed->name = parse_node_name((char**)pointer);

    DeviceTreeProperty* prop = NULL;
    while (1) {
        skip_nops(pointer);
        prop = parse_prop(header, pointer);
        if (prop == NULL) {
            break;
        }
        add_linked_prop(&parsed->properties, prop);
    }

    PRINTF_IF(DEBUG_DEVICE_TREE, "done with props\n");

    DeviceTreeNode* child = NULL;
    while (1) {
        skip_nops(pointer);
        child = parse_node(header, pointer);
        if (child == NULL) {
            break;
        }
        add_linked_node(&parsed->child, child);
    }

    PRINTF_IF(DEBUG_DEVICE_TREE, "done with children\n");

    skip_nops(pointer);

    if (**pointer != STRUCTURE_END_NODE) {
        PANIC("unclosed device tree node");
    }
    PRINTF_IF(DEBUG_DEVICE_TREE, "END_NODE\n");

    *pointer += 1;

    return parsed;
}

DeviceTree parse_device_tree(const DeviceTreeHeadersRaw* header) {
    if (header->magic != DEVICE_TREE_MAGIC) {
        PANIC("bad magic on device tree header: 0x%08x",
              BIG_TO_LITTLE32(header->magic));
    }
    if (header->version != DEVICE_TREE_VERSION &&
        header->last_comp_version != DEVICE_TREE_VERSION) {
        PANIC("bad version/comp_version on device tree header: %d",
              BIG_TO_LITTLE32(header->version));
    }

    const uint32_t* pointer =
        (void*)((uintptr_t)header + BIG_TO_LITTLE32(header->off_dt_struct));

    DeviceTreeNode* root = parse_node(header, &pointer);

    if (*pointer != STRUCTURE_END) {
        PANIC("device tree structure not closed");
    }

    if (root == NULL) {
        PANIC("device tree NULL root node");
    }

    DeviceTree tree = {
        .boot_cpuid_phys = BIG_TO_LITTLE32(header->boot_cpuid_phys),
        .root = root,
    };

    return tree;
}

// indent at depth
#define D(x) printf("%*s", (x) * 4, "");

DeviceTreeProperty* DeviceTreeNode_find_property(DeviceTreeNode* node,
                                                 char* property_name) {
    if (node == NULL) {
        return NULL;
    }

    DeviceTreeProperty* prop = node->properties;
    while (prop != NULL) {
        if (0 == strcmp(prop->name, property_name)) {
            return prop;
        }
        prop = prop->next;
    }
    return NULL;
}

// find a device tree node based on path
// path should start with DEVICE_TREE_ROOT_PATH if starting at root
// path must end with a NULL
// depth should be 0 if starting at root
DeviceTreeNode* DeviceTreeNode_find_child(DeviceTreeNode* node, char* path[],
                                          uint8_t depth) {
    DeviceTreeNode* check = NULL;

    if (DEBUG_DEVICE_TREE_SEARCH) {
        D(depth) printf("node \"%s\"\n", node->name);
    }

    // if path matches
    if (0 == strcmp(node->name, *(path + depth))) {
        // don't search any more!
        if (*(path + depth + 1) == NULL) {
            if (DEBUG_DEVICE_TREE_SEARCH) {
                D(depth) printf("match!\n");
            }
            return node;
        } else {
            if (DEBUG_DEVICE_TREE_SEARCH) {
                D(depth) printf("subpath match\n");
            }
        }

        // check child with next path
        if (node->child != NULL) {
            if (DEBUG_DEVICE_TREE_SEARCH) {
                D(depth) printf("checking child\n");
            }
            check = DeviceTreeNode_find_child(node->child, path, depth + 1);

            if (check != NULL) {
                return check;
            }
        }
    }

    // check sibling
    if (node->next != NULL) {
        check = DeviceTreeNode_find_child(node->next, path, depth);

        if (check != NULL) {
            return check;
        }
    }

    return NULL;
}

void DeviceTreeNode_print(DeviceTreeNode* node, uint8_t depth) {
    D(depth) printf("DeviceTreeNode {\n");

    DeviceTreeProperty* prop = node->properties;
    while (prop != NULL) {
        D(depth + 1) printf("DeviceTreeProperty { ");
        printf("name: \"%s\", ", prop->name);
        printf("data: [%d] ", prop->value_len);
        for (size_t i = 0; i < prop->value_len; i++) {
            printf("%02x ", ((char*)prop->value)[i]);
        }
        printf("},\n");

        prop = prop->next;
    }

    DeviceTreeNode* child = node->child;
    while (child != NULL) {
        DeviceTreeNode_print(child, depth + 1);
        child = child->next;
    }

    D(depth) printf("},\n");
}

void DeviceTree_print(DeviceTree* tree) {
    printf("DeviceTree {\n");
    D(1) printf("boot_cpuid_phys: %x,\n", tree->boot_cpuid_phys);
    DeviceTreeNode_print(tree->root, 1);
    printf("}\n");
}

// dump entire blob
// xxd -p -r $PLAINTEXT_FILE > $BINARY_FILE
void DeviceTree_dump_raw(const DeviceTreeHeadersRaw* header) {
    printf("raw header address: %p\n", header);
    printf("--- start DeviceTree dump ---\n");

    for (uintptr_t i = 0; i < BIG_TO_LITTLE32(header->totalsize); i++) {
        char* this_ptr = (void*)((uintptr_t)header + i);
        if (i != 0 && i % 16 == 0) {
            printf("\n");
        }
        printf("%02x ", (char)*this_ptr);
    }
    printf("\n");
    printf("--- end DeviceTree dump ---\n");
}
