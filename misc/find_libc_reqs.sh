#!/bin/sh

grep -nr src/libc -e "extern" \
    | sed -nE 's/(.*):(.*)/\1\t\2/p' \
    | sort
