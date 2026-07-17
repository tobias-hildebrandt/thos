#!/bin/sh
# $1 outfile
# $* qemu exec and args
OUTFILE="$1"; shift
COMMAND=$*

echo "---COMMAND: $COMMAND---" > "$OUTFILE"

PIPE=$(mktemp -u -p build/).fifo

mkfifo "$PIPE"

# qemu outputs \r\n instead of \n
# stdbuf needed to force tr to have unbuffered output
stdbuf -o0 tr -d '\r' < "$PIPE" | tee -a "$OUTFILE" &

$COMMAND > "$PIPE" 2>&1
EXITCODE=$?

# shellcheck disable=SC2086
rm -f "$PIPE"

echo "---QEMU EXIT CODE: $EXITCODE---" | tee -a "$OUTFILE"
