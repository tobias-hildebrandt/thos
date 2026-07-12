#!/bin/sh
if [ -n "$LIBGCC_OVERRIDE" ]; then
    echo "$LIBGCC_OVERRIDE"
    exit 0
fi

if [ -z "$MACHINE_ARGS" ]; then
    exit 1
fi

# use default
GCC_CROSS=${GCC_CROSS:-"riscv64-unknown-elf-gcc"}

# shellcheck disable=SC2086
${GCC_CROSS} ${MACHINE_ARGS} -print-libgcc-file-name

