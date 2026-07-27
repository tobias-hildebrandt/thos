#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "test.h"

#define SPRINTF_TEST(len, expected, format_str, ...)                       \
    char buffer[64];                                                       \
    int printed = sprintf(buffer, (format_str)__VA_OPT__(, ) __VA_ARGS__); \
    TEST_ASSERT_EQ(printed, (len), "%d");                                  \
    TEST_ASSERT_EQ(strlen(buffer), (len), "%d");                           \
    TEST_ASSERT_EQ(strnlen_s(buffer, 64), (len), "%d");                    \
    TEST_ASSERT_EQ(0, strcmp(buffer, (expected)), "%d")

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

TEST(snprintf_less) {
    char buffer[32];
    memset(buffer, 255, 32);
    int printed = snprintf(buffer, 32, "hello");
    TEST_ASSERT(printed == 5);
}

TEST(snprintf_same) {
    char buffer[16];
    memset(buffer, 255, 16);
    int printed = snprintf(buffer, 16, "1234567890abcde");
    TEST_ASSERT_EQ(printed, 15, "%d");
    TEST_ASSERT_EQ(0, strcmp(buffer, "1234567890abcde"), "%d");
    TEST_ASSERT_EQ(buffer[15], 0, "%d");
}

TEST(snprintf_one_more) {
    char buffer[16];
    memset(buffer, 255, 16);
    int printed = snprintf(buffer, 16, "1234567890abcdef");
    TEST_ASSERT_EQ(printed, 16, "%d");
    TEST_ASSERT_EQ(0, strcmp(buffer, "1234567890abcde"), "%d");
    TEST_ASSERT_EQ(buffer[15], 0, "%d");
}

TEST(snprintf_more) {
    char buffer[8];
    memset(buffer, 255, 8);
    int printed = snprintf(buffer, 8, "1234567890abcde");
    TEST_ASSERT_EQ(printed, 15, "%d");
    TEST_ASSERT_EQ(0, strcmp(buffer, "1234567"), "%d");
    TEST_ASSERT_EQ(buffer[7], 0, "%d");
}

TEST(snprintf_zero) {
    char buffer[16];
    memset(buffer, 255, 16);
    int printed = snprintf(buffer, 0, "1234567890abcde");
    TEST_ASSERT_EQ(printed, 15, "%d");
    for (int i = 0; i < 16; i++) {
        TEST_ASSERT_EQ(buffer[i], 255, "%d");
    }
}
