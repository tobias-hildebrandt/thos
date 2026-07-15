#!/bin/sh

if [ ! -d "subprojects" ]; then
    echo "this script should be run from project root"
    exit 1
fi

if [ "$1" = "-d" ] || [ "$1" = "--dry-run" ]; then
    PREFIX="echo"
fi

# relinks meson subproject wrap packagefiles into the subproject
# $1: subproject name
# $2: filename (in packagefiles/$1/) to link
relink_file() {
    $PREFIX ln -sf "$PWD/subprojects/packagefiles/$1/$2" "$PWD/subprojects/$1/$2"
}

# $1: subproject name
# $*: filenames to relink
relink_many_files() {
    SUBPROJECT=$1; shift
    for file in "$@"; do
        relink_file "$SUBPROJECT" "$file"
    done
}

relink_many_files "opensbi" "meson.build" "meson.options"


