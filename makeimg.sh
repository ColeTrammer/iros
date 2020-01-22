#!/bin/sh

dd if=/dev/zero of=os_2.img bs=516096c count=1000

losetup -o0 /dev/loop100 os_2.img
mke2fs -b1024 /dev/loop100

mkdir -p mnt
mount -text2 /dev/loop100 mnt

cp -r sysroot/* mnt
mkdir -p mnt/home/eloc
mkdir -p mnt/etc
touch mnt/etc/resolv.conf

cat > mnt/etc/hosts << __HOSTS__
localhost 127.0.0.1
__HOSTS__
cat > mnt/etc/passwd << __PASSWD__
root:x:0:0:root:/:/bin/sh
eloc:x:100:100:eloc,,,:/home/eloc:/bin/sh
__PASSWD__
cat > mnt/etc/group << __GROUP__
root:x:0:
eloc:x:100:
__GROUP__

ln -s /bin/ls mnt/home/eloc/ls

umount /dev/loop100
losetup -d /dev/loop100

chmod 777 os_2.img