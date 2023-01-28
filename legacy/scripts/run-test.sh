#!/bin/sh

if [ "$NATIVE" = TRUE ]; then
    exec "$@"
fi

export ROOT="${IROS_ROOT:-$(realpath ..)}"
ARCH="${IROS_ARCH:-x86_64}"

TEST_PATH="$1"
TEST_NAME="$(basename $TEST_PATH)"

shift 1
TEST_ARGS=$(echo "$@" | tr " " '|')

if [ "$TEST_ARGS" ]; then
    TEST_ARGS="start_args=$TEST_ARGS"
fi

CMDLINE="poweroff_on_panic=1;redirect_start_stdio_to_serial=1;start=/bin/$TEST_NAME;$TEST_ARGS"
if [ $IROS_QUIET_KERNEL ]; then
    CMDLINE="disable_serial_debug=1;$CMDLINE"
fi

if [ $IROS_REPORT_STATUS ]; then
    ! IROS_CMDLINE="$CMDLINE" \
    IROS_DISABLE_GRAPHICS=1 \
    "$ROOT/scripts/qemu.sh" \
    | tee /dev/stderr \
    | grep -q FAIL
else
    IROS_CMDLINE="$CMDLINE" \
    IROS_DISABLE_GRAPHICS=1 \
    "$ROOT/scripts/qemu.sh"
fi

