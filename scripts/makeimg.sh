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

DEFAULT_DISK_SIZE=100m
if [ -e "$IROS_GRUB_IMAGE" ]; then
    DEFAULT_DISK_SIZE=120m;
fi

DISK_SIZE="${IROS_DISK_SIZE:-$DEFAULT_DISK_SIZE}"

qemu-img create "$OUTPUT_DIR/iros.img" "$DISK_SIZE"
if [ -e "$IROS_GRUB_IMAGE" ]; then
    parted -s -- "$OUTPUT_DIR/iros.img" \
        mklabel gpt \
        mkpart BIOSBOOT ext3 1Mib 8Mib \
        mkpart IROS ext2 8Mib -34s \
        set 1 bios_grub
    ROOT_PARTITION=p2
else
    parted -s -- "$OUTPUT_DIR/iros.img" \
        mklabel gpt \
        mkpart IROS ext2 1Mib -34s
    ROOT_PARTITION=p1
fi

cleanup() {
    umount "$OUTPUT_DIR/mnt"
    losetup -d "$LOOP_DEV"
}
trap cleanup EXIT

LOOP_DEV=$(losetup --partscan -f "$OUTPUT_DIR/iros.img" --show)

# HACK to make partitions show up in a docker container
# https://github.com/moby/moby/issues/27886#issuecomment-417074845
# drop the first line, as this is our LOOP_DEV itself, but we only want the child partitions
PARTITIONS=$(lsblk --raw --output "MAJ:MIN" --noheadings ${LOOP_DEV} | tail -n +2)
COUNTER=1
for i in $PARTITIONS; do
    MAJ=$(echo $i | cut -d: -f1)
    MIN=$(echo $i | cut -d: -f2)
    if [ ! -e "${LOOP_DEV}p${COUNTER}" ]; then mknod ${LOOP_DEV}p${COUNTER} b $MAJ $MIN; fi
    COUNTER=$((COUNTER + 1))
done

mke2fs "$LOOP_DEV$ROOT_PARTITION"

mkdir -p "$OUTPUT_DIR/mnt"
mount -text2 "$LOOP_DEV$ROOT_PARTITION" "$OUTPUT_DIR/mnt"

cp -r --preserve=mode,links $ROOT/base/* "$OUTPUT_DIR/mnt"
cp -r --preserve=mode,links $SYSROOT/* "$OUTPUT_DIR/mnt"
chown -R 50:50 "$OUTPUT_DIR/mnt/home/test"
chown -R 100:100 "$OUTPUT_DIR/mnt/home/iris"

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

chmod 777 "$OUTPUT_DIR/iros.img"

if [ -e "$IROS_GRUB_IMAGE" ]; then
    grub-install --boot-directory="$OUTPUT_DIR/mnt/boot" --target=i386-pc --modules="ext2 part_msdos" "$LOOP_DEV"

    cp "$IROS_GRUB_IMAGE" "$OUTPUT_DIR/mnt/boot/grub/grub.cfg"

    mkdir -p "$OUTPUT_DIR/mnt/modules"
    cp "$SYSROOT/boot/initrd.bin" "$OUTPUT_DIR/mnt/modules/"
fi
