#!/bin/sh
SOURCE_DIR=$1
PRIVATE_DIR=$2
FILE_TO_COPY=$3
FILE_OUT=$4

# set output dir env var
export O="$PRIVATE_DIR"

LOGFILE="$PRIVATE_DIR/make-log.txt"

# TODO: only append if make didn't output "nothing to be done"
date -Isecond >> "$LOGFILE"
# args already set in env
make -C "$SOURCE_DIR" | tee -a "$LOGFILE"

BASE='platform/generic/firmware'

ln -sf "$PRIVATE_DIR/$BASE/$FILE_TO_COPY" "$OUT_DIR/$FILE_OUT"
