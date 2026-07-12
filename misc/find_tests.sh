#!/bin/sh
OUT=$1; shift

printf "" > "$OUT"

for file in "$@"; do
    llvm-nm "$file" \
        | grep _test_ \
        | sed -nE 's/[0-9a-f]* [^ ]* (_test_[a-z0-9_-]*)/\1/p' \
        >> "$OUT"
done
