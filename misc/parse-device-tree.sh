#!/bin/sh
# shellcheck disable=SC2059

KERNEL_OUTFILE=$1
HEX_OUT=$2
BINARY_OUT=$3
HUMAN_OUT=$4

PRINTF_FMT="%-16s %s\n"

set -e

sed -n '/--- start DeviceTree dump ---/, /--- end DeviceTree dump ---/p' \
    "$KERNEL_OUTFILE" | grep -v -e 'DeviceTree dump' > "$HEX_OUT"
printf "$PRINTF_FMT" "$HEX_OUT" "extracted hex dump from outfile"

xxd -r -p "$HEX_OUT" > "$BINARY_OUT"
printf "$PRINTF_FMT" "$BINARY_OUT" "decoded binary device-tree from dump"

dtc "$BINARY_OUT" > "$HUMAN_OUT"
printf "$PRINTF_FMT" "$HUMAN_OUT" "decoded human-readable device-tree from binary"
