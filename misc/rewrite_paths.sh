#!/bin/sh
PIPE_WRAPPER="$(command -v pipetty)"

## regex match:
# - start of line
# - group 1:
#   - nothing
# - OR
#   - character that isnt [
#   - any number of characters that aren't .
# - ../../src/
## replaces with:
# - group 1
# - src/

PIPE=$(mktemp -u -p build/).fifo

mkfifo "$PIPE"

# launch sed to read from pipe
sed -E 's%^(|[^\[][^.]*)\.\./\.\./src/%\1\src/%gm' < "$PIPE" &

# run command into pipe
# shellcheck disable=SC2048,SC2086
$PIPE_WRAPPER $* > "$PIPE"
EXITCODE=$?

rm -f "$PIPE"

# return with same exit code as command
exit $EXITCODE

