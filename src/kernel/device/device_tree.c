#include "device/device_tree.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "align.h"
#include "endian.h"
#include "flags.h"
#include "io.h"
#include "list.h"
#include "panic.h"

enum { DEVICE_TREE_VERSION = BIG_TO_LITTLE32(17) };

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

enum { NUM_DEVICE_TREE_PROPERTIES = 1024 };
enum { NUM_DEVICE_TREE_NODES = 256 };

ALLOCATE_ARRAY_AND_COUNTER(nodes, DeviceTreeNode, NUM_DEVICE_TREE_NODES);
ALLOCATE_ARRAY_AND_COUNTER(props, DeviceTreeProperty,
                           NUM_DEVICE_TREE_PROPERTIES);

static void skip_nops(const uint32_t** pointer) {
    while (**pointer == STRUCTURE_NOP) {
        PRINTF_IF(DEBUG_DEVICE_TREE, "skipping NOP\n");
        *pointer += 1;
    }
}

static char* parse_node_name(char** pointer) {
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

static DeviceTreeProperty* DeviceTreeProperty_parse(
    const DeviceTreeHeadersRaw* header, const uint32_t** pointer) {
    skip_nops(pointer);

    if (**pointer != STRUCTURE_PROP) {
        return NULL;
    }
    PRINTF_IF(DEBUG_DEVICE_TREE, "PROP @ %p, offset = %#x\n", *pointer,
              (uintptr_t)*pointer - (uintptr_t)header);

    *pointer += 1;

    DeviceTreeProperty* parsed =
        NEXT_FREE(props, DeviceTreeProperty, NUM_DEVICE_TREE_NODES);

    DeviceTreeStructurePropertyRaw* property_raw =
        (DeviceTreeStructurePropertyRaw*)*pointer;

    // move past raw property
    *pointer =
        (void*)(((char*)*pointer) + sizeof(DeviceTreeStructurePropertyRaw));

    // grab name via header offset
    parsed->name = ((char*)header + BIG_TO_LITTLE32(header->off_dt_strings) +
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
    *pointer = (void*)((char*)*pointer + parsed->value_len);

    // align
    *pointer = (void*)(align_up((uintptr_t)*pointer, 4));

    PRINTF_IF(DEBUG_DEVICE_TREE, "post-prop pointer: %p\n", *pointer);

    return parsed;
}

static DeviceTreeNode* DeviceTreeNode_parse(const DeviceTreeHeadersRaw* header,
                                            const uint32_t** pointer) {
    skip_nops(pointer);

    if (**pointer != STRUCTURE_BEGIN_NODE) {
        return NULL;
    }
    PRINTF_IF(DEBUG_DEVICE_TREE, "BEGIN_NODE\n");
    *pointer += 1;

    DeviceTreeNode* parsed =
        NEXT_FREE(nodes, DeviceTreeNode, NUM_DEVICE_TREE_NODES);

    parsed->name = parse_node_name((char**)pointer);

    DeviceTreeProperty* prop = NULL;
    while (1) {
        skip_nops(pointer);
        prop = DeviceTreeProperty_parse(header, pointer);
        if (prop == NULL) {
            break;
        }
        ADD_LINKED(parsed->properties, prop, DeviceTreeProperty, next);
    }

    PRINTF_IF(DEBUG_DEVICE_TREE, "done with props\n");

    DeviceTreeNode* child = NULL;
    while (1) {
        skip_nops(pointer);
        child = DeviceTreeNode_parse(header, pointer);
        if (child == NULL) {
            break;
        }
        ADD_LINKED(parsed->child, child, DeviceTreeNode, next);
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

DeviceTree DeviceTree_parse(const DeviceTreeHeadersRaw* header) {
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
        (void*)((char*)header + BIG_TO_LITTLE32(header->off_dt_struct));

    DeviceTreeNode* root = DeviceTreeNode_parse(header, &pointer);

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
static DeviceTreeNode* DeviceTreeNode_find_child_recursive(DeviceTreeNode* node,
                                                           DeviceTreePath path,
                                                           uint8_t depth) {
    DeviceTreeNode* check = NULL;

    if (DEBUG_DEVICE_TREE_SEARCH) {
        D(depth) printf("node \"%s\"\n", node->name);
    }

    // if path matches
    if (0 == strcmp(node->name, *(path.path + depth))) {
        // don't search any more!
        if (*(path.path + depth + 1) == NULL) {
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
            check = DeviceTreeNode_find_child_recursive(node->child, path,
                                                        depth + 1);

            if (check != NULL) {
                return check;
            }
        }
    }

    // check sibling
    if (node->next != NULL) {
        check = DeviceTreeNode_find_child_recursive(node->next, path, depth);

        if (check != NULL) {
            return check;
        }
    }

    return NULL;
}

DeviceTreeNode* DeviceTreeNode_find_child(DeviceTreeNode* node,
                                          DeviceTreePath path) {
    return DeviceTreeNode_find_child_recursive(node, path, 0);
}

static void DeviceTreeNode_print(DeviceTreeNode* node, uint8_t depth) {
    printf("DeviceTreeNode {\n");
    D(depth + 1) printf("name: \"%s\",\n", node->name);

    int counter = 0;

    DeviceTreeProperty* prop = node->properties;
    while (prop != NULL) {
        D(depth + 1) printf("prop[%d]: ", counter);
        printf("DeviceTreeProperty { ");
        printf("name: \"%s\", ", prop->name);
        printf("data: ");

        // check if value could be a string
        // (doesnt check for multiple nul-terminated strings)
        const bool ends_with_nul =
            (((char*)prop->value)[prop->value_len - 1]) == 0;
        bool normal_ascii = true;
        bool all_zeros = true;
        bool first_is_not_zero =
            prop->value_len > 0 ? ((char*)prop->value)[0] != 0 : false;
        for (size_t i = 0; i < prop->value_len; i++) {
            char value = ((char*)prop->value)[i];
            // just check for non-control normal ascii
            if ((value < 0x20 || value > 0x7e) && value != 0 &&
                !(value == '\n' || value == '\t' || value == '\r')) {
                normal_ascii = false;
                break;
            }
            if (value != 0) {
                all_zeros = false;
            }
        }

        if (normal_ascii && !all_zeros && ends_with_nul && first_is_not_zero) {
            // print as string
            printf("\"%s\" ", prop->value);
        }
        // print as hex
        printf("[%d] ", prop->value_len);

        for (size_t i = 0; i < prop->value_len; i++) {
            char value = ((char*)prop->value)[i];
            printf("%02x ", value);
        }

        printf("},\n");

        prop = prop->next;
        counter += 1;
    }

    counter = 0;
    DeviceTreeNode* child = node->child;
    while (child != NULL) {
        D(depth + 1) printf("child[%d]: ", counter);
        DeviceTreeNode_print(child, depth + 1);
        child = child->next;
        counter += 1;
    }

    D(depth) printf("},\n");
}

void DeviceTree_print(DeviceTree* tree) {
    printf("DeviceTree {\n");
    D(1) printf("boot_cpuid_phys: %x,\n", tree->boot_cpuid_phys);
    D(1) DeviceTreeNode_print(tree->root, 1);
    printf("}\n");
}

// dump entire blob
// xxd -p -r $PLAINTEXT_FILE > $BINARY_FILE
void DeviceTree_dump_raw(const DeviceTreeHeadersRaw* header) {
    printf("raw header address: %p\n", header);
    printf("--- start DeviceTree dump ---\n");

    for (uintptr_t i = 0; i < BIG_TO_LITTLE32(header->totalsize); i++) {
        char* this_ptr = (char*)header + i;
        if (i != 0 && i % 16 == 0) {
            printf("\n");
        }
        printf("%02x ", *this_ptr);
    }
    printf("\n");
    printf("--- end DeviceTree dump ---\n");
}
