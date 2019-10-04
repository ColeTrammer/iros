#!/bin/sh

dd if=/dev/zero of=ext2_test.img bs=516096c count=200

losetup -o0 /dev/loop100 ext2_test.img
mke2fs -b1024 /dev/loop100

mkdir -p mnt
mount -text2 /dev/loop100 mnt

cd mnt

mkdir -p "test/sub"

echo "ccccc" > test/sub/c.txt
echo "bbbbb" > test/b.txt
echo "aaaaa" > a.txt

sync

cd ..

umount /dev/loop100
losetup -d /dev/loop100

chmod 777 ext2_test.img