#!/bin/sh

ELF=$1
OUT=$2
shift 2
ARGS=$*
OBJCOPY=${OBJCOPY:-llvm-objcopy}

# user BIN (fully-expanded memory image)
# shellcheck disable=SC2086
${OBJCOPY} $ARGS --output-target=binary "$ELF" "$OUT"
