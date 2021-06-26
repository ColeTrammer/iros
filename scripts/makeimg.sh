#!/bin/sh

set -e

if [ "$(id -u)" != 0 ];
then
    echo "this script must be ran as root"
    exit 1
fi

qemu-img create scripts/os_2.img 200m
parted -s -- scripts/os_2.img \
    mklabel gpt \
    mkpart P1 ext2 1Mib -34s

LOOP_DEV=$(losetup -o 1048576 --sizelimit=$((199 * 1048576 - 34 * 512)) -f scripts/os_2.img --show)
mke2fs "$LOOP_DEV"

cleanup() {
    umount "$LOOP_DEV"
    losetup -d "$LOOP_DEV"
}
trap cleanup EXIT

mkdir -p mnt
mount -text2 "$LOOP_DEV" mnt

cp -r --preserve=mode,links base/* mnt
cp -r --preserve=mode,links sysroot/* mnt
chown -R 50:50 mnt/home/test
chown -R 100:100 mnt/home/eloc

chmod u+s mnt/bin/ping
chmod u+s mnt/bin/su

ln -s grep mnt/bin/egrep
ln -s grep mnt/bin/fgrep
ln -s grep mnt/bin/rgrep

ln -s chown mnt/bin/chgrp

mkdir mnt/dev
mkdir mnt/initrd
mkdir mnt/proc
mkdir mnt/tmp

mknod mnt/dev/null c 1 1 -m 666
mknod mnt/dev/zero c 1 2 -m 666
mknod mnt/dev/full c 1 3 -m 666
mknod mnt/dev/urandom c 1 4 -m 666
mknod mnt/dev/ptmx c 2 1 -m 666
mknod mnt/dev/tty c 2 2 -m 666
mknod mnt/dev/sda b 5 0 -m 660
mknod mnt/dev/sda1 b 5 1 -m 660
mknod mnt/dev/fb0 c 6 0 -m 660
mknod mnt/dev/serial0 c 8 0 -m 222

chgrp 14 mnt/dev/fb0

ln -s /proc/self/fd mnt/dev/fd
ln -s /proc/self/fd/0 mnt/dev/stdin
ln -s /proc/self/fd/1 mnt/dev/stdout
ln -s /proc/self/fd/2 mnt/dev/stderr
ln -s urandom mnt/dev/random

chmod 777 scripts/os_2.img
