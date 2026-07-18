#!/bin/sh

find src/ \
    \( \
    -name '*.c' \
    -or \
    -name '*.h' \
    \) \
    -exec echo "formatting {}" \; \
    -exec clang-format -i {} \;
