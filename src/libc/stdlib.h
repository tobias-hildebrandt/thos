#pragma once

int atoi(const char* str);
long atol(const char* str);
long long atoll(const char* str);

// NOLINTBEGIN(modernize-macro-to-enum)
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
// NOLINTEND(modernize-macro-to-enum)

extern void exit(int exit_code);
