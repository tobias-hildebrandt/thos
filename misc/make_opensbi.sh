#!/bin/sh
# wrapper around opensbi's Makefile
SOURCE_DIR=$1
BUILD_DIR=$2
MAKE_TARGET=$3 # relative to BUILD_DIR
FILE_OUT=$4
VERBOSE=$5

BUILD_DIR="$(readlink -f "$BUILD_DIR")"
FILE_OUT="$(readlink -f "$FILE_OUT")"

# set output dir env var
export O="$BUILD_DIR"

mkdir -p "$BUILD_DIR"

LOGFILE="$BUILD_DIR/make-log.txt"

# TODO: only append if make didn't output "nothing to be done"
date -Isecond >> "$LOGFILE"

FULL_TARGET="$BUILD_DIR/$MAKE_TARGET"

# TODO: pipe failures
# args already set in env
if [ "$VERBOSE" = "1" ]; then
    {
    echo "PWD:          $PWD"
    echo "O:            $O"
    echo "FULL_TARGET:  $FULL_TARGET"
    echo "CC:           $CC"
    echo "cc:           $(which cc)"
    echo make -C "$SOURCE_DIR" "$FULL_TARGET"
    }  | tee -a "$LOGFILE"
    make -C "$SOURCE_DIR" "$FULL_TARGET"  | tee -a "$LOGFILE" 2>&1
else
    make -C "$SOURCE_DIR" "$FULL_TARGET" >> "$LOGFILE" 2>&1
fi

cp "$BUILD_DIR/$MAKE_TARGET" "$FILE_OUT"

