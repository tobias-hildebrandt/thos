#include "trap.h"

#include "io.h"  // IWYU pragma: keep, needed for printf
#include "sbi.h"

struct TrapFrame {
    // return address, global pointer, thread pointer
    uint64_t ra, gp, tp;
    // temporaries
    uint64_t t0, t1, t2, t3, t4, t5, t6;
    // arguments
    uint64_t a0, a1, a2, a3, a4, a5, a6, a7;
    // saved
    uint64_t s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;
    // stack pointer
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
        REGISTER_STACK(sd, ra, 0)
        REGISTER_STACK(sd, gp, 1)
        REGISTER_STACK(sd, tp, 2)
        // t registers
        REGISTER_STACK_OFFSET(sd, T_OFFSET, t, 0)
        REGISTER_STACK_OFFSET(sd, T_OFFSET, t, 1)
        REGISTER_STACK_OFFSET(sd, T_OFFSET, t, 2)
        REGISTER_STACK_OFFSET(sd, T_OFFSET, t, 3)
        REGISTER_STACK_OFFSET(sd, T_OFFSET, t, 4)
        REGISTER_STACK_OFFSET(sd, T_OFFSET, t, 5)
        REGISTER_STACK_OFFSET(sd, T_OFFSET, t, 6) //9
        // a registers
        REGISTER_STACK_OFFSET(sd, A_OFFSET, a, 0)
        REGISTER_STACK_OFFSET(sd, A_OFFSET, a, 1)
        REGISTER_STACK_OFFSET(sd, A_OFFSET, a, 2)
        REGISTER_STACK_OFFSET(sd, A_OFFSET, a, 3)
        REGISTER_STACK_OFFSET(sd, A_OFFSET, a, 4)
        REGISTER_STACK_OFFSET(sd, A_OFFSET, a, 5)
        REGISTER_STACK_OFFSET(sd, A_OFFSET, a, 6)
        REGISTER_STACK_OFFSET(sd, A_OFFSET, a, 7) //17
        // s registers
        REGISTER_STACK_OFFSET(sd, S_OFFSET, s, 0)
        REGISTER_STACK_OFFSET(sd, S_OFFSET, s, 1)
        REGISTER_STACK_OFFSET(sd, S_OFFSET, s, 2)
        REGISTER_STACK_OFFSET(sd, S_OFFSET, s, 3)
        REGISTER_STACK_OFFSET(sd, S_OFFSET, s, 4)
        REGISTER_STACK_OFFSET(sd, S_OFFSET, s, 5)
        REGISTER_STACK_OFFSET(sd, S_OFFSET, s, 6)
        REGISTER_STACK_OFFSET(sd, S_OFFSET, s, 7)
        REGISTER_STACK_OFFSET(sd, S_OFFSET, s, 8)
        REGISTER_STACK_OFFSET(sd, S_OFFSET, s, 9)
        REGISTER_STACK_OFFSET(sd, S_OFFSET, s, 10)
        REGISTER_STACK_OFFSET(sd, S_OFFSET, s, 11) //29
        // read old stack pointer
        "csrr a0, sscratch\n"
        // push it onto stack
        REGISTER_STACK(sd, a0, 30)
        // clang-format on
        // a0 (first function argument) points to TrapFrame on stack
        "mv a0, sp\n"
        "call " STRINGIFY(handle_trap) "\n"

        // after call, load back registers

        // clang-format off
        // starting registers
        REGISTER_STACK(ld, ra, 0)
        REGISTER_STACK(ld, gp, 1)
        REGISTER_STACK(ld, tp, 2)
        // t registers
        REGISTER_STACK_OFFSET(ld, T_OFFSET, t, 0)
        REGISTER_STACK_OFFSET(ld, T_OFFSET, t, 1)
        REGISTER_STACK_OFFSET(ld, T_OFFSET, t, 2)
        REGISTER_STACK_OFFSET(ld, T_OFFSET, t, 3)
        REGISTER_STACK_OFFSET(ld, T_OFFSET, t, 4)
        REGISTER_STACK_OFFSET(ld, T_OFFSET, t, 5)
        REGISTER_STACK_OFFSET(ld, T_OFFSET, t, 6) //9
        // a registers
        REGISTER_STACK_OFFSET(ld, A_OFFSET, a, 0)
        REGISTER_STACK_OFFSET(ld, A_OFFSET, a, 1)
        REGISTER_STACK_OFFSET(ld, A_OFFSET, a, 2)
        REGISTER_STACK_OFFSET(ld, A_OFFSET, a, 3)
        REGISTER_STACK_OFFSET(ld, A_OFFSET, a, 4)
        REGISTER_STACK_OFFSET(ld, A_OFFSET, a, 5)
        REGISTER_STACK_OFFSET(ld, A_OFFSET, a, 6)
        REGISTER_STACK_OFFSET(ld, A_OFFSET, a, 7) //17
        // s registers
        REGISTER_STACK_OFFSET(ld, S_OFFSET, s, 0)
        REGISTER_STACK_OFFSET(ld, S_OFFSET, s, 1)
        REGISTER_STACK_OFFSET(ld, S_OFFSET, s, 2)
        REGISTER_STACK_OFFSET(ld, S_OFFSET, s, 3)
        REGISTER_STACK_OFFSET(ld, S_OFFSET, s, 4)
        REGISTER_STACK_OFFSET(ld, S_OFFSET, s, 5)
        REGISTER_STACK_OFFSET(ld, S_OFFSET, s, 6)
        REGISTER_STACK_OFFSET(ld, S_OFFSET, s, 7)
        REGISTER_STACK_OFFSET(ld, S_OFFSET, s, 8)
        REGISTER_STACK_OFFSET(ld, S_OFFSET, s, 9)
        REGISTER_STACK_OFFSET(ld, S_OFFSET, s, 10)
        REGISTER_STACK_OFFSET(ld, S_OFFSET, s, 11) //29
        // restore old stack pointer
        REGISTER_STACK(ld, sp, 30)
        // (technically could just subtract, would avoid a load op)
        // "addi sp, sp, %[frame_size]\n"
        // clang-format on

        "sret\n"
        :  // no output
        : [frame_size] "i"(sizeof(TrapFrame))

    );
}
