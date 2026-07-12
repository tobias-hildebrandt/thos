#!/bin/sh

if [ "$1" = "-f" ] || [ "$1" = "--full" ]; then
    FULL="1"
    shift
fi

KERNEL_SOURCE=$1

if [ "$FULL" = "1" ]; then
    find "$KERNEL_SOURCE" -type f -exec \
        sed -nE \
        -e 's|^TEST\(([[:alnum:]_]*)\).*|_test__\1|p' \
        -e 's|^TEST_SHOULD_FAIL\(([[:alnum:]_]*)\).*|_test__\1_should_fail|p' \
        {} \;
else
    find "$KERNEL_SOURCE" -type f -exec \
        sed -nE \
        -e 's|^TEST[A-Z_]*\(([[:alnum:]_]*)\).*|\1|p' \
        {} \;
fi
