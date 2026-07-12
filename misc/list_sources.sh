#!/bin/sh
if [ -z "$TARGET" ]; then
    exit 1
fi

find "$TARGET" -name '*.c'
