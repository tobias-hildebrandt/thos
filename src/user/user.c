const char text[] = "this is some example text\n";

const char lotta_space[4196];

int main(void) {
    volatile int sum = 0;
    while (1) {
        sum += 1;  // text[i];
    }
    return sum;
}

extern char __stack_top[];

__attribute__((section(".text.start"))) __attribute__((naked)) void start(
    void) {
    __asm__ __volatile__(
        "mv sp, %[stack_top]\n"
        "j main\n" ::[stack_top] "r"(__stack_top));
}
