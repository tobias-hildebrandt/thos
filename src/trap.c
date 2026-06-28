#include "trap.h"

#include "io.h"  // IWYU pragma: keep, needed for printf
#include "sbi.h"

// TODO: push frame onto stack in trap_vector and read it in handle_trap

struct TrapFrame {
    uint64_t ra;
    uint64_t gp;
    uint64_t tp;
    uint64_t t0;
    uint64_t t1;
    uint64_t t2;
    uint64_t t3;
    uint64_t t4;
    uint64_t t5;
    uint64_t t6;
    uint64_t a0;
    uint64_t a1;
    uint64_t a2;
    uint64_t a3;
    uint64_t a4;
    uint64_t a5;
    uint64_t a6;
    uint64_t a7;
    uint64_t s0;
    uint64_t s1;
    uint64_t s2;
    uint64_t s3;
    uint64_t s4;
    uint64_t s5;
    uint64_t s6;
    uint64_t s7;
    uint64_t s8;
    uint64_t s9;
    uint64_t s10;
    uint64_t s11;
    uint64_t sp;
} __attribute__((packed));
typedef struct TrapFrame TrapFrame;

void handle_trap(TrapFrame* frame) {
    uint64_t scause;
    uint64_t stval;
    uint64_t sepc;

    (void)frame;

    __asm__ __volatile__("csrr %0, scause" : "=r"(scause));
    __asm__ __volatile__("csrr %0, stval" : "=r"(stval));
    __asm__ __volatile__("csrr %0, sepc" : "=r"(sepc));

    // TODO: actually handle traps
    // TODO: these are actually longs, need to implement lx
    printf(
        "kernel trap\n"
        "scause: 0x%x, stval: 0x%x, sepc: 0x%x\n",
        scause, stval, sepc);

    // TODO: determine which exceptions are fatal and which are recoverable

    if (scause == 0x2 && (stval & 0xffffffff) == 0xc0001073) {
        printf("skipping unimp!\n");

        // skip offending instruction
        // size is always 4 bytes, since we do NOT have compressed instructions
        __asm__ __volatile__("csrw sepc, %0" ::"r"(sepc + 4));
    } else {
        printf("fatal trap, shutting down\n");
        sbi_shutdown();
    };
}

// macros for asm string creation
#define REGISTER_STACK(instr, reg, offset) \
    #instr " " #reg ", 8 * (" #offset ")(sp)\n"
#define REGISTER_STACK_OFFSET(instr, start_offset, prefix, x) \
    REGISTER_STACK(instr, prefix##x, start_offset + x)

#define T_OFFSET 3
#define A_OFFSET (T_OFFSET + 7)
#define S_OFFSET (A_OFFSET + 8)

__attribute__((naked)) __attribute__((aligned(4))) void trap_vector(void) {
    __asm__ __volatile__(
        // store current stack pointer in scratch
        "csrw sscratch, sp\n"
        // add space on stack
        "addi sp, sp, -%[frame_size]\n"

        // store registers
        // clang-format off
        // starting registers
        REGISTER_STACK(sw, ra, 0)
        REGISTER_STACK(sw, gp, 1)
        REGISTER_STACK(sw, tp, 2)
        // t registers
        REGISTER_STACK_OFFSET(sw, T_OFFSET, t, 0)
        REGISTER_STACK_OFFSET(sw, T_OFFSET, t, 1)
        REGISTER_STACK_OFFSET(sw, T_OFFSET, t, 2)
        REGISTER_STACK_OFFSET(sw, T_OFFSET, t, 3)
        REGISTER_STACK_OFFSET(sw, T_OFFSET, t, 4)
        REGISTER_STACK_OFFSET(sw, T_OFFSET, t, 5)
        REGISTER_STACK_OFFSET(sw, T_OFFSET, t, 6) //9
        // a registers
        REGISTER_STACK_OFFSET(sw, A_OFFSET, a, 0)
        REGISTER_STACK_OFFSET(sw, A_OFFSET, a, 1)
        REGISTER_STACK_OFFSET(sw, A_OFFSET, a, 2)
        REGISTER_STACK_OFFSET(sw, A_OFFSET, a, 3)
        REGISTER_STACK_OFFSET(sw, A_OFFSET, a, 4)
        REGISTER_STACK_OFFSET(sw, A_OFFSET, a, 5)
        REGISTER_STACK_OFFSET(sw, A_OFFSET, a, 6)
        REGISTER_STACK_OFFSET(sw, A_OFFSET, a, 7) //17
        // s registers
        REGISTER_STACK_OFFSET(sw, S_OFFSET, s, 0)
        REGISTER_STACK_OFFSET(sw, S_OFFSET, s, 1)
        REGISTER_STACK_OFFSET(sw, S_OFFSET, s, 2)
        REGISTER_STACK_OFFSET(sw, S_OFFSET, s, 3)
        REGISTER_STACK_OFFSET(sw, S_OFFSET, s, 4)
        REGISTER_STACK_OFFSET(sw, S_OFFSET, s, 5)
        REGISTER_STACK_OFFSET(sw, S_OFFSET, s, 6)
        REGISTER_STACK_OFFSET(sw, S_OFFSET, s, 7)
        REGISTER_STACK_OFFSET(sw, S_OFFSET, s, 8)
        REGISTER_STACK_OFFSET(sw, S_OFFSET, s, 9)
        REGISTER_STACK_OFFSET(sw, S_OFFSET, s, 10)
        REGISTER_STACK_OFFSET(sw, S_OFFSET, s, 11) //29
        // read old stack pointer
        "csrr a0, sscratch\n"
        // push it onto stack
        REGISTER_STACK(sw, a0, 30)
        // clang-format on
        // a0 (first function argument) points to TrapFrame on stack
        "mv a0, sp\n"
        "call " STRINGIFY(handle_trap) "\n"

        // after call, load back registers

        // clang-format off
        // starting registers
        REGISTER_STACK(lw, ra, 0)
        REGISTER_STACK(lw, gp, 1)
        REGISTER_STACK(lw, tp, 2)
        // t registers
        REGISTER_STACK_OFFSET(lw, T_OFFSET, t, 0)
        REGISTER_STACK_OFFSET(lw, T_OFFSET, t, 1)
        REGISTER_STACK_OFFSET(lw, T_OFFSET, t, 2)
        REGISTER_STACK_OFFSET(lw, T_OFFSET, t, 3)
        REGISTER_STACK_OFFSET(lw, T_OFFSET, t, 4)
        REGISTER_STACK_OFFSET(lw, T_OFFSET, t, 5)
        REGISTER_STACK_OFFSET(lw, T_OFFSET, t, 6) //9
        // a registers
        REGISTER_STACK_OFFSET(lw, A_OFFSET, a, 0)
        REGISTER_STACK_OFFSET(lw, A_OFFSET, a, 1)
        REGISTER_STACK_OFFSET(lw, A_OFFSET, a, 2)
        REGISTER_STACK_OFFSET(lw, A_OFFSET, a, 3)
        REGISTER_STACK_OFFSET(lw, A_OFFSET, a, 4)
        REGISTER_STACK_OFFSET(lw, A_OFFSET, a, 5)
        REGISTER_STACK_OFFSET(lw, A_OFFSET, a, 6)
        REGISTER_STACK_OFFSET(lw, A_OFFSET, a, 7) //17
        // s registers
        REGISTER_STACK_OFFSET(lw, S_OFFSET, s, 0)
        REGISTER_STACK_OFFSET(lw, S_OFFSET, s, 1)
        REGISTER_STACK_OFFSET(lw, S_OFFSET, s, 2)
        REGISTER_STACK_OFFSET(lw, S_OFFSET, s, 3)
        REGISTER_STACK_OFFSET(lw, S_OFFSET, s, 4)
        REGISTER_STACK_OFFSET(lw, S_OFFSET, s, 5)
        REGISTER_STACK_OFFSET(lw, S_OFFSET, s, 6)
        REGISTER_STACK_OFFSET(lw, S_OFFSET, s, 7)
        REGISTER_STACK_OFFSET(lw, S_OFFSET, s, 8)
        REGISTER_STACK_OFFSET(lw, S_OFFSET, s, 9)
        REGISTER_STACK_OFFSET(lw, S_OFFSET, s, 10)
        REGISTER_STACK_OFFSET(lw, S_OFFSET, s, 11) //29
        // old stack pointer (don't restore, just subtract)
        // REGISTER_STACK(lw, sp, 30)
        // clang-format on
        "addi sp, sp, %[frame_size]\n"

        "sret\n"
        :  // no output
        : [frame_size] "i"(sizeof(TrapFrame))

    );
}
