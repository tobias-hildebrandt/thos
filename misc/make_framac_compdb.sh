#!/bin/sh
INPUT=$1
OUTPUT=$2
REPLACEMENT_DIR=$(realpath "$3")

# replace -I (except -I/) with -I$REPLACEMENT_DIR/
# replace -isystem (except -isystem/) with -isystem$REPLACEMENT_DIR/

sed -E \
    -e "s|-I([^/])|-I$REPLACEMENT_DIR/\1|g" \
    -e "s|-isystem([^/])|-isystem$REPLACEMENT_DIR/\1|g" \
    < "$INPUT" \
    > "$OUTPUT"
