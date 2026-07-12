#!/bin/sh
OUT=$1; shift

TESTS="$*"
NUM_TESTS=$#

{
cat <<EOF
// file generated via $0

#include "test.h"
#include <string.h>
#include <stddef.h>

const size_t num_tests = $NUM_TESTS;

EOF

for test in $TESTS; do
    echo "void $test(void);"
done
echo

echo "const Test tests_data[$NUM_TESTS] = {"
for test in $TESTS; do
    echo "    { .name = \"$test\", .func = $test },"
done
echo "};"

echo "const Test* tests = (const Test*) &tests_data;"
} > "$OUT"
