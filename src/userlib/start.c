
#include "asm.h"
#include "util.h"

extern char __stack_top[];
extern char main[];

UNUSED NAKED SECTION(".text.start") void start(void) {
    ASM("mv sp, %[stack_top]\n"
        "j main\n" ::[stack_top] "r"(__stack_top));
}
