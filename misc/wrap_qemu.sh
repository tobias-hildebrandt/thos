#!/bin/sh
# $1 outfile
# $* qemu exec and args
OUTFILE="$1"; shift
COMMAND=$*
PIPE=$(mktemp -u -p build/).fifo

mkfifo "$PIPE"

{
    # qemu outputs \r\n instead of \n
    tr -d '\r' |tee -a "$OUTFILE"
} < "$PIPE" &
PIPEPID=$!

$COMMAND > "$PIPE"
EXITCODE=$?

kill -9 $PIPEPID
rm -f "$PIPE"

echo "---QEMU EXIT CODE: $EXITCODE---" | tee -a "$OUTFILE"
