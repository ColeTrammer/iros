#!/bin/sh

set -e

PARENT_DIR=$(realpath "$(dirname -- "$0")")
IROS_ROOT=$(realpath "$PARENT_DIR"/..)
IROS_BUILD_DIR=${IROS_BUILD_DIR:-$(realpath .)}

die() {
    echo "$@"
    exit 1
}

if [ $# = 0 ]; then
    die "error: this script must be passed at least 1 argument"
fi

cd "$IROS_ROOT"

tidy_args="-p ${IROS_BUILD_DIR} -use-color 1 -allow-no-checks -quiet"

action="$1"
shift

case $action in
tidy)
    tidy_args="$tidy_args -fix -format"
    ;;
analyze)
    tidy_args="$tidy_args -checks=-*,clang-analyzer-*,-clang-analyzer-cplusplus*"
    ;;
check) ;;
*) die "error: action '$action' is not valid" ;;
esac

while getopts ":s:a:" opt; do
    case $opt in
    s)
        tidy_args="$tidy_args -source-filter $OPTARG"
        ;;
    a)
        tidy_args="$tidy_args $OPTARG"
        ;;
    \?)
        die "Invalid option: -$OPTARG"
        ;;
    esac
done

set -x
! run-clang-tidy $tidy_args 2>&1 | grep -vE '^$|Applying fixes|clang-tidy|[[:digit:]]+ warnings? generated'
