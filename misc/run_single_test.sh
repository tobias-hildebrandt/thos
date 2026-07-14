#!/bin/sh
ARGS=$*
PARSE_EXITCODE=0

parse_args() {
    if [ "$1" = "--parse-exitcode" ] || [ "$1" = "-p" ]; then
        PARSE_EXITCODE=1
        shift
    fi
    COMMAND=$*
}

# shellcheck disable=SC2086
parse_args $ARGS


if [ -z "$COMMAND" ]; then
    echo "must pass command via args"
    exit 1
fi

TEMP=$(mktemp)

sh -c "$COMMAND" > "$TEMP"
EXIT=$?

if [ $PARSE_EXITCODE -eq 1 ]; then
    EXIT="$(sed -nE 's|---EXITCODE ([0-9]+)---|\1|p' "$TEMP")"
fi

cat "$TEMP" | tr -d '\r' # qemu outputs \r\n instead of \n
rm "$TEMP"

exit "$EXIT"
