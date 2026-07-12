#!/bin/sh

ELF=$1
OUT=$2
OBJCOPY=${OBJCOPY:-llvm-objcopy}

# user BIN (fully-expanded memory image)
${OBJCOPY} --set-section-flags \*=alloc,contents --output-target=binary "$ELF" "$OUT"
