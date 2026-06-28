/* linker script */

OUTPUT_ARCH("riscv")

/* entrypoint is boot() function */
ENTRY(boot)

SECTIONS {
    /* start at this address */
    . = 0x80200000;

    /* code lives here */
    .text : {
        /* boot function goes first */
        *(.text.boot);
        /* all other code */
        *(.text .text.*);
    }

    /* readonly data */
    .rodata : ALIGN(16) {
        *(.rodata .rodata.*);
    }

    /* globals */
    .data : ALIGN(16) {
        *(.data .data.*);
    }

    /* 0-initialized globals */
    .bss : ALIGN(16) {
        *(.bss .bss.* .sbss .sbss.*);
    } = 0x0

    /* align */
    . = ALIGN(16);

    /* stack size */
    . += 64K;

    __STACK_START = .;
}
