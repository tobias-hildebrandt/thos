#!/bin/sh

TARGETS=${TARGETS:-"riscv64 riscv32"}
TOOLCHAINS=${TOOLCHAINS:-"llvm gnu"}
MACHINES=${MACHINES:-"virt sifive_u"}

OUT="build/test-matrix-out.txt"
TEST_RESULTS="build/test-matrix-results.txt"

printf "" > "$OUT"
printf "" > "$TEST_RESULTS"

for TARGET in $TARGETS; do
    for TOOLCHAIN in $TOOLCHAINS; do
        for MACHINE in $MACHINES; do
            test_log_file="build/"$TARGET-$TOOLCHAIN-$MACHINE"/meson-logs/testlog.txt"
            echo "-----------------------"
            echo "TESTING: $TARGET $TOOLCHAIN $MACHINE" | tee -a "$TEST_RESULTS"
            make test TARGET="$TARGET" TOOLCHAIN="$TOOLCHAIN" MACHINE="$MACHINE" >> "$OUT"
            echo "$test_log_file" | tee -a "$TEST_RESULTS"
            cat "$test_log_file" >> "$TEST_RESULTS"
            tail -n6 "$test_log_file"
        done
    done
done
