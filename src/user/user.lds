ENTRY(start)

SECTIONS {
    . = 0x1000000;

    .text :{
        *(.text.start);
        *(.text .text.*);
    }

    .rodata : ALIGN(16) {
        *(.rodata .rodata.*);
    }

    .data : ALIGN(16) {
        *(.data .data.*);
    }

    .bss : ALIGN(16) {
        *(.bss .bss.* .sbss .sbss.*);
    } = 0x0

    .stack : ALIGN(16) {
        . += 64 * 1024;
        __stack_top = .;
    }
}
