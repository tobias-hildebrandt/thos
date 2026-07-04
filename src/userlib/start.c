
extern char __stack_top[];
extern char main[];

__attribute__((section(".text.start"))) __attribute__((naked)) void start(
    void) {
    __asm__ __volatile__(
        "mv sp, %[stack_top]\n"
        "j main\n" ::[stack_top] "r"(__stack_top));
}
