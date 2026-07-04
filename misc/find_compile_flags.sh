#!/bin/sh

grep -nr src/ -e "#ifndef" \
    | sed -nE 's/(.*):#ifndef (.*)/\1\t\2/p' \
    | sort
