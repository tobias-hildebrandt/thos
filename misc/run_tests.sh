#!/bin/sh

# test harness that follows test anything protocol https://testanything.org/

# reads from $*
COMMAND=$*

if [ -z "$COMMAND" ]; then
    echo "must pass command via args"
    exit 1
fi

len() {
    echo $#
}

run_qemu() {
    TEMP=$(mktemp)
    # shellcheck disable=SC2086 # allow expansion
    sh -c "$COMMAND -append $1" > "$TEMP"
    EXIT=$?
    cat "$TEMP" | tr -d '\r' # qemu outputs \r\n instead of \n
    rm "$TEMP"
    return $EXIT
}

clean_test_name() {
    echo "$1" | sed -e 's/_test__//' -e 's/_should_fail//'
}

echo "TAP version 14"
echo
echo "# $(date -Iseconds)"
echo "# COMMAND: $COMMAND"

echo
TESTS=$(run_qemu "--tests")

# shellcheck disable=SC2086 # allow expansion
NUM_TESTS=$(len $TESTS)

echo "1..${NUM_TESTS}"
echo

i=1
for test_raw in $TESTS; do
    should_fail=0
    test_clean=$(clean_test_name "$test_raw")
    if echo "$test_raw" | grep -q "_should_fail"; then
        should_fail=1
    fi
    # run test
    OUT=$(run_qemu "--test$test_raw")
    EXIT_CODE=$?

    # print info in comments
    echo "# test: $test_clean"
    echo "# raw test function: $test_raw"
    echo "# exit_code: $EXIT_CODE"
    echo "# output:"
    echo "$OUT" | sed -E 's|^(.*)|## \1|'
    if [ "$EXIT_CODE" = "77" ]; then
        # skip
        printf "ok %d # SKIP" $i
    elif { [ "$EXIT_CODE" = "0" ] && [ $should_fail = "0" ]; } \
        || { [ "$EXIT_CODE" != "0" ] && [ $should_fail = "1" ]; } then
        # normal and passed
        # or
        # expect-fail and fail
        printf "ok %d" $i
    else
        printf "not ok %d" $i
    fi

    printf " - %s\n" "$test_clean"
    echo

    i=$((i+1))
done
