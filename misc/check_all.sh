#!/bin/sh
FILES=$(find src/ \( -name '*.c' -or -name '*.h' \))

# shellcheck disable=SC2086 # need to split
clang-tidy \
    --config-file=.clang-tidy \
    -p build/compile_commands.json \
    -header-filter=.* \
    $FILES
