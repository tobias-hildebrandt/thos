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
