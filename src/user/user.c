#include <stdio.h>

#include "syscall.h"

const char text[] =
    "0123456789abcdefghijklmnopqrstuvwxyz"
    "!@#$%^&*()_+";

const char lotta_space[4196];

int main(void) {
    int index = 0;
    while (1) {
        yield();
        if (text[index] == '\0') {
            index = 0;
        }

        printf("hello %c\n", text[index]);

        index += 1;
    }

    return 0;
}

extern char __stack_top[];

__attribute__((section(".text.start"))) __attribute__((naked)) void start(
    void) {
    __asm__ __volatile__(
        "mv sp, %[stack_top]\n"
        "j main\n" ::[stack_top] "r"(__stack_top));
}
