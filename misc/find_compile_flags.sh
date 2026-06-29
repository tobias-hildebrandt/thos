#!/bin/sh

grep -nr src/ -e "#ifndef" \
    | sed -nE 's/(.*):#ifndef (.*)/\1 \2/p' \
    | sort
