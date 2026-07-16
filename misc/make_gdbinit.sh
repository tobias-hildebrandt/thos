#!/bin/sh
OUTPUT=$1
TEMPLATE=$2
GDB_PORT=$3
TARGET=$4

# TODO: move in inferiors and default inferior

# only keep digits
ARCH=$(echo "$TARGET" | tr -d -c '[:digit:]')

sed \
    -e "s/:PORT_GOES_HERE/:$GDB_PORT/" \
    -e "s/ARCH_GOES_HERE/$ARCH/" \
    < "$TEMPLATE" > "$OUTPUT"
