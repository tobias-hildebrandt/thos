#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "test.h"

#define SPRINTF_TEST(len, expected, format_str, ...)                       \
    char buffer[64];                                                       \
    int printed = sprintf(buffer, (format_str)__VA_OPT__(, ) __VA_ARGS__); \
    TEST_ASSERT(printed == (len));                                         \
    TEST_ASSERT(strlen(buffer) == (len));                                  \
    TEST_ASSERT(strnlen_s(buffer, 64) == (len));                           \
    TEST_ASSERT(0 == strcmp(buffer, (expected)))

TEST(sprintf_simple) {
    SPRINTF_TEST(7, "hello 1", "hello 1", NULL);
}
TEST(sprintf_decimal_positive) {
    SPRINTF_TEST(9, "hello 123", "hello %d", 123);
}
TEST(sprintf_decimal_negative) {
    SPRINTF_TEST(10, "hello -123", "hello %d", -123);
}

TEST(strlen_simple) {
    char* string = "hello world";
    TEST_ASSERT(strlen(string) == 11);
    TEST_ASSERT(strnlen_s(string, 16) == 11);
}

TEST(strnlen_s_simple) {
    TEST_ASSERT(strnlen_s("hello one two three longer string", 5) == 5);
}
