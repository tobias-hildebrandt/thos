#!/bin/sh
COMMAND=$*

if [ -z "$COMMAND" ]; then
    echo "must pass command via args"
    exit 1
fi

TEMP=$(mktemp)

sh -c "$COMMAND" > "$TEMP"
EXIT=$?

cat "$TEMP" | tr -d '\r' # qemu outputs \r\n instead of \n
rm "$TEMP"

exit $EXIT
