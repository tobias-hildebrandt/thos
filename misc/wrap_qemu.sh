#!/bin/sh
# $* qemu exec and args
COMMAND=$*
PIPE="build/qemu-out.pipe"
OUTFILE="build/out"

rm -f $PIPE
mkfifo $PIPE

tee $OUTFILE < $PIPE &
PIPEPID=$!

$COMMAND > $PIPE
EXITCODE=$?

rm -f $PIPE

echo "(QEMU EXIT CODE: $EXITCODE)" | tee -a $OUTFILE

