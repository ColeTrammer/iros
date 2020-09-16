#!/bin/sh

set -e

if [ "$(id -u)" != 0 ];
then
    echo "this script must be ran as root"
    exit 1
fi

qemu-img create os_2.img 200m
mke2fs os_2.img

mkdir -p mnt
mount -text2 os_2.img mnt

cp -r --preserve=mode base/* mnt
cp -r --preserve=mode sysroot/* mnt
chown -R 50:50 mnt/home/test
chown -R 100:100 mnt/home/eloc

chmod u+s mnt/bin/ping
chmod u+s mnt/bin/su

ln -s grep mnt/bin/egrep
ln -s grep mnt/bin/fgrep
ln -s grep mnt/bin/rgrep

ln -s chown mnt/bin/chgrp

mkdir mnt/dev
mknod mnt/dev/null c 1 1 -m 666
mknod mnt/dev/zero c 1 2 -m 666
mknod mnt/dev/full c 1 3 -m 666
mknod mnt/dev/urandom c 1 4 -m 666
mknod mnt/dev/ptmx c 2 1 -m 666
mknod mnt/dev/tty c 2 2 -m 666
mknod mnt/dev/hdd0 b 5 0 -m 660
mknod mnt/dev/fb0 c 6 0 -m 660
mknod mnt/dev/keyboard c 7 1 -m 440
mknod mnt/dev/mouse c 7 2 -m 440
mknod mnt/dev/serial0 c 8 0 -m 222

chgrp 13 mnt/dev/keyboard mnt/dev/mouse
chgrp 14 mnt/dev/fb0

ln -s /proc/self/fd mnt/dev/fd
ln -s /proc/self/fd/0 mnt/dev/stdin
ln -s /proc/self/fd/1 mnt/dev/stdout
ln -s /proc/self/fd/2 mnt/dev/stderr
ln -s urandom mnt/dev/random

umount os_2.img

chmod 777 os_2.img
