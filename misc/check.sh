#!/bin/sh

if [ "$1" = "-o" ] || [ "$1" = "--out" ]; then
    shift;
    OUT=$1
    shift;
    echo "teeing to $OUT"
    printf "" > "$OUT"
fi

if [ $# -eq 0 ]; then
    FILES=$(find src/ \( -name '*.c' -or -name '*.h' \))
else
    FILES=$*
fi

check() {
    echo "checking file $file"
    misc/rewrite_paths.sh \
    clang-tidy \
        --config-file=.clang-tidy \
        -p build/compile_commands.json \
        -header-filter='src/.*' \
        "$file" 2>&1
    echo
}

for file in $FILES; do
    if [ -n "$OUT" ]; then
        check "$file" | ansi2txt | tee -a "$OUT"
    else
        check "$file"
    fi
done
