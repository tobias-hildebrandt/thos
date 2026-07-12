#include <stdio.h>
#include <stdlib.h>

#define NUM_MESSAGES 8
char* messages[NUM_MESSAGES] = {
    // clang-format off
    "hello world",
    "this is from a process",
    "a user process",
    "it is running",
    "but not for long",
    "...",
    "now it will shut down",
    "goodbye cruel world"
    // clang-format on
};

const char lotta_space[4196];

int main(void) {
    for (int i = 0; i < NUM_MESSAGES; i++) {
        printf("user: %s\n", messages[i]);
    }
    exit(0);
    return 0;
}
