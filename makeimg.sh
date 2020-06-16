#!/bin/sh

set -e

if [ "$(id -u)" != 0 ];
then
    echo "this script must be ran as root"
    exit 1
fi

dd if=/dev/zero of=os_2.img bs=516096c count=400

losetup -o0 /dev/loop100 os_2.img
mke2fs /dev/loop100

mkdir -p mnt
mount -text2 /dev/loop100 mnt

cp -r --preserve=mode base/* mnt
cp -r --preserve=mode sysroot/* mnt
chown -R 50:50 mnt/home/test
chown -R 100:100 mnt/home/eloc

chmod u+s mnt/bin/ping
chmod u+s mnt/bin/su

ln -s grep mnt/bin/egrep
ln -s grep mnt/bin/fgrep
ln -s grep mnt/bin/rgrep

mkdir mnt/dev
mknod mnt/dev/serial c 0x003 0xF8
mknod mnt/dev/fb0 c 0x012 0x34 -m 777
mknod mnt/dev/ptmx c 0x075 0x00 -m 777
mknod mnt/dev/hdd0 b 0x004 0x30
mknod mnt/dev/keyboard c 0x000 0x20 -m 777
mknod mnt/dev/mouse c 0x005 0x00 -m 777

ln -s /proc/self/fd mnt/dev/fd
ln -s /proc/self/fd/0 mnt/dev/stdin
ln -s /proc/self/fd/1 mnt/dev/stdout
ln -s /proc/self/fd/2 mnt/dev/stderr

umount /dev/loop100
losetup -d /dev/loop100

chmod 777 os_2.img
