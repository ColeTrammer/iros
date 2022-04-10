#!/bin/sh

set -e

if [ "$(id -u)" != 0 ];
then
    echo "this script must be ran as root"
    exit 1
fi

if [ ! "$OUTPUT_DIR" ] || [ ! "$ROOT" ] || [ ! "$SYSROOT" ];
then
    echo "OUTPUT_DIR, ROOT, SYSROOT all must be set"
    exit 1
fi

qemu-img create "$OUTPUT_DIR/os_2.img" 30m
parted -s -- "$OUTPUT_DIR/os_2.img" \
    mklabel gpt \
    mkpart P1 ext2 1Mib -34s

LOOP_DEV=$(losetup -o 1048576 --sizelimit=$((199 * 1048576 - 34 * 512)) -f "$OUTPUT_DIR/os_2.img" --show)
mke2fs "$LOOP_DEV"

cleanup() {
    umount "$LOOP_DEV"
    losetup -d "$LOOP_DEV"
}
trap cleanup EXIT

mkdir -p "$OUTPUT_DIR/mnt"
mount -text2 "$LOOP_DEV" "$OUTPUT_DIR/mnt"

cp -r --preserve=mode,links $ROOT/base/* "$OUTPUT_DIR/mnt"
cp -r --preserve=mode,links $SYSROOT/* "$OUTPUT_DIR/mnt"
chown -R 50:50 "$OUTPUT_DIR/mnt/home/test"
chown -R 100:100 "$OUTPUT_DIR/mnt/home/eloc"

chmod u+s "$OUTPUT_DIR/mnt/bin/ping"
chmod u+s "$OUTPUT_DIR/mnt/bin/su"

ln -s grep "$OUTPUT_DIR/mnt/bin/egrep"
ln -s grep "$OUTPUT_DIR/mnt/bin/fgrep"
ln -s grep "$OUTPUT_DIR/mnt/bin/rgrep"

ln -s chown $OUTPUT_DIR"/mnt/bin/chgrp"

mkdir "$OUTPUT_DIR/mnt/dev"
mkdir "$OUTPUT_DIR/mnt/initrd"
mkdir "$OUTPUT_DIR/mnt/proc"
mkdir "$OUTPUT_DIR/mnt/tmp"

mknod "$OUTPUT_DIR/mnt/dev/null" c 1 1 -m 666
mknod "$OUTPUT_DIR/mnt/dev/zero" c 1 2 -m 666
mknod "$OUTPUT_DIR/mnt/dev/full" c 1 3 -m 666
mknod "$OUTPUT_DIR/mnt/dev/urandom" c 1 4 -m 666
mknod "$OUTPUT_DIR/mnt/dev/ptmx" c 2 1 -m 666
mknod "$OUTPUT_DIR/mnt/dev/tty" c 2 2 -m 666
mknod "$OUTPUT_DIR/mnt/dev/sda" b 5 0 -m 660
mknod "$OUTPUT_DIR/mnt/dev/sda1" b 5 1 -m 660
mknod "$OUTPUT_DIR/mnt/dev/fb0" c 6 0 -m 660
mknod "$OUTPUT_DIR/mnt/dev/serial0" c 8 0 -m 222

chgrp 14 "$OUTPUT_DIR/mnt/dev/fb0"

ln -s /proc/self/fd "$OUTPUT_DIR/mnt/dev/fd"
ln -s /proc/self/fd/0 "$OUTPUT_DIR/mnt/dev/stdin"
ln -s /proc/self/fd/1 "$OUTPUT_DIR/mnt/dev/stdout"
ln -s /proc/self/fd/2 "$OUTPUT_DIR/mnt/dev/stderr"
ln -s urandom "$OUTPUT_DIR/mnt/dev/random"

chmod 777 "$OUTPUT_DIR/os_2.img"
