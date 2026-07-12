#include <stddef.h>
#include <string.h>

void* memset(void* buf, char c, size_t n) {
    for (size_t i = 0; i < n; i++) {
        ((char*)buf)[i] = c;
    }
    return buf;
}

void* memcpy(void* dst, const void* src, size_t n) {
    for (size_t i = 0; i < n; i++) {
        ((char*)dst)[i] = ((char*)src)[i];
    }

    return dst;
}

int strcmp(const char* lhs, const char* rhs) {
    if (lhs == NULL && rhs == NULL) {
        return 0;
    } else if (lhs == NULL && rhs != NULL) {
        return 1;
    } else if (lhs != NULL && rhs == NULL) {
        return -1;
    }
    while (1) {
        char left = *lhs;
        char right = *rhs;

        if (left == 0 && right != 0) {
            return -1;
        } else if (left != 0 && right == 0) {
            return 1;
        } else if (left == 0 && right == 0) {
            return 0;
        } else {
            char diff = left - right;
            if (diff != 0) {
                return diff;
            }
            lhs += 1;
            rhs += 1;
        }
    }
}

// TODO: dedeuplicate from strcmp
int strncmp(const char* lhs, const char* rhs, size_t count) {
    if (count == 0) {
        return 0;
    }

    if (lhs == NULL && rhs == NULL) {
        return 0;
    } else if (lhs == NULL && rhs != NULL) {
        return 1;
    } else if (lhs != NULL && rhs == NULL) {
        return -1;
    }

    for (size_t i = 0; i < count; i++) {
        char left = *(lhs + i);
        char right = *(rhs + i);

        if (left == 0 && right != 0) {
            return -1;
        } else if (left != 0 && right == 0) {
            return 1;
        } else if (left == 0 && right == 0) {
            return 0;
        } else {
            char diff = left - right;
            if (diff != 0) {
                return diff;
            }
        }
    }
    return 0;
}
