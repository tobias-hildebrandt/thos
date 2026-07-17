#!/bin/sh
# script to make and manage pipes for qemu serial chardev


DO_CREATE=0
DO_DESTROY=0
DO_REDIRECT=0

# linux getopt
getopt --test > /dev/null \
    && { echo "not linux getopt"; exit 1; }
ARGS=$(getopt --name "$0" --shell 'sh' --options 'cdr' --longoptions 'create,destroy,redirect' -- "$@")
EXITVAL=$?
if [ $EXITVAL -ne 0 ]; then
    echo "getopt returned $EXITVAL"
    exit $EXITVAL
fi

eval set -- "$ARGS"

while true; do case "$1" in
    -c | --create)      DO_CREATE=1; shift;;
    -d | --destroy)     DO_DESTROY=1; shift;;
    -r | --redirect)    DO_REDIRECT=1; shift;;
    --)                 shift; break;;
    *)                  echo "unknown argument: $1"; exit 1;;
esac done

if  [ $DO_CREATE -eq 0 ] \
    && [ $DO_DESTROY -eq 0 ] \
    && [ $DO_REDIRECT -eq 0 ]; then
    DO_CREATE=1
    DO_DESTROY=1
    DO_REDIRECT=1
fi

PATH_BASE=$1

if [ -z "$PATH_BASE" ]; then
    echo "must pass path base as positional argument" 1>&2
    exit 1
fi

PIPE_IN="$PATH_BASE.in"
PIPE_OUT="$PATH_BASE.out"

destroy() {
    rm -f "$PIPE_IN" "$PIPE_OUT"
}

create() {
    for pipe in "$PIPE_IN" "$PIPE_OUT"; do
        # make pipe unless file already exists and is a pipe
        if ! [ -p "$pipe" ] && ! mkfifo "$pipe"; then
            echo "failed to create pipe" 1>&2
            exit 1
        fi
    done
}

redirect() {
    # pipe stdout in background
    { while true; do stdbuf -i0 -o0 cat "$PIPE_OUT"; done; } &

    # pipe stdin in foreground
    while true; do stdbuf -i0 -o0 cat - 2>/dev/null > "$PIPE_IN"; done;
}

trap_cleanup() {
    trap "destroy" INT QUIT ABRT TERM
}

main() {
    if [ $DO_CREATE -eq 1 ]; then
        create
    fi
    if [ $DO_REDIRECT -eq 1 ]; then
        if [ $DO_DESTROY -eq 1 ]; then
            trap_cleanup
        fi
        redirect
    fi
    if [ $DO_DESTROY -eq 1 ]; then
        destroy
    fi
}

main
