#!/bin/sh

set -e

LOCKFILE="/tmp/.run-command-on-iris.lock"
LOCK_TIMEOUT_SECONDS=30

die() {
    echo "$@"
    exit 1
}

# Acquire exclusive lock, because we need to create a kernel image and initrd to run the program.
# If concurrent execution is allowed, we would need to make arbitrary numbers of images, which is undesirable as images
# are 64 MiB each.
exec 9>$LOCKFILE || die "Could not open exclusive lock."
flock -w "$LOCK_TIMEOUT_SECONDS" 9 || die "Could not acquire exclusive lock."
trap "rm -f $LOCKFILE" EXIT

PARENT_DIR=$(realpath $(dirname -- "$0"))
export IROS_ROOT=$(realpath "$PARENT_DIR"/..)

export IROS_ARCH=x86_64

echo "Building for $IROS_ARCH..."
echo "IROS_ROOT=$IROS_ROOT"
echo "IROS_ARCH=$IROS_ARCH"
echo "IROS_BUILD_DIR=$IROS_BUILD_DIR"
echo "IROS_IMAGE=$IROS_IMAGE"
echo "IROS_INITRD=$IROS_INITRD"
echo "1=$1"

export IROS_BUILD_DIR="$IROS_ROOT/build/$1"
export IROS_LIMINE_DIR="$IROS_ROOT"/build/host/tools/limine/src

export IROS_IMAGE="$IROS_BUILD_DIR"/iris-run.img
export IROS_INITRD="$IROS_BUILD_DIR"/initrd-run/initrd.bin

mkdir -p "$IROS_BUILD_DIR"/initrd-run
rm -f "$IROS_INITRD"

if [ "$2" = "-run=kernel_unit_test" ]; then
    EXECUTABLE_NAME="$2"
else
    cp "$2"* "$IROS_BUILD_DIR"/initrd-run
    EXECUTABLE_NAME="/$(basename $2)"
fi

(
    cd "$IROS_BUILD_DIR"/initrd-run
    "$IROS_ROOT"/build/host/tools/install/bin/initrd
)


export IROS_LIMINE_CFG="$IROS_BUILD_DIR/initrd-run/limine.cfg"
cat >"$IROS_LIMINE_CFG" << __EOF__
TIMEOUT=0

:Iros
    PROTOCOL=limine

    KERNEL_PATH=boot:///iris
    KERNEL_CMDLINE=$EXECUTABLE_NAME

    MODULE_PATH=boot:///initrd.bin
__EOF__

# Desperately try up to 5 times to make the image.
FAILED='true'
for i in `seq 5`; do
    if [ "$FAILED" = 'true'  ]; then
        if sudo -E "$PARENT_DIR"/make-iris-limine-image.sh; then
            FAILED='false'
        fi
    fi
done

if [ "$FAILED" = 'true' ]; then
    echo "Failed to create disk image."
    exit 1
fi

set +e

"$PARENT_DIR"/run-iris.sh

STATUS=$?
if [ ! "$STATUS" -eq 33 ]; then
    die "Status is not success, is instead" "$STATUS"
fi

exit 0
