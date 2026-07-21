/* linker script */

OUTPUT_ARCH("riscv")

/* entrypoint is boot() function */
ENTRY(boot)

/* TODO: guard pages */

SECTIONS {
    /* normal memory starts at this address */
    . = 0x80200000;

    __MEMORY_START = .;

    /* program code */
    __TEXT_START = .;
    .text : {
        /* boot function goes first */
        *(.text.boot);
        *(.text .text.*);
    }
    __TEXT_END = .;

    /* readonly data */
    __RODATA_START = .;
    .rodata : ALIGN(16) {
        *(.rodata .rodata.*);
    }
    __RODATA_END = .;

    /* globals */
    __DATA_START = .;
    .data : ALIGN(16) {
        *(.data .data.*);
    }
    __DATA_END = .;

    /* 0-initialized globals */
    __BSS_START = .;
    .bss : ALIGN(16) {
        *(.bss .bss.* .sbss .sbss.*);
    } = 0x0
    __BSS_END = .;

    /* global special section */
    . = ALIGN(4096);
    __GLOBAL_SPECIAL_START = .;
    .global_special : ALIGN(4096) {
        *(.global_special);
    } = 0x0
    . = ALIGN(4096);
    __GLOBAL_SPECIAL_END = .;

    /* user special section */
    . = ALIGN(4096);
    __USER_SPECIAL_START = .;
    .user_special : ALIGN(4096) {
        *(.user_special);
    }
    . = ALIGN(4096);
    __USER_SPECIAL_END = .;

    /* stack (grows down) */
    . = ALIGN(16);
    __STACK_START = .;
    . += 64K;
    __STACK_END = .;

    /* pages for allocation */
    . = ALIGN(4096);
    __PAGES_START = .;
    . += 256 * 4096;
    __PAGES_END = .;

    __MEMORY_END = .;
}
