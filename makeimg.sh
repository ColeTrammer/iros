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

umount /dev/loop100
losetup -d /dev/loop100

chmod 777 os_2.img
