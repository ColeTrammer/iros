#!/bin/sh

set -e

if [ "$(id -u)" != 0 ];
then
    echo "This script must be ran as root."
    exit 1
fi

if [ ! "$IROS_BUILD_DIR" ] || [ ! "$IROS_ROOT" ] || [ ! "$IROS_LIMINE_DIR" ];
then
    echo "IROS_BUILD_DIR, IROS_ROOT, IROS_LIMINE_DIR all must be set."
    exit 1
fi

echo "$IROS_LIMINE_DIR"

DEFAULT_DISK_SIZE=64m
DISK_SIZE="${IROS_DISK_SIZE:-$DEFAULT_DISK_SIZE}"

IMAGE="$IROS_BUILD_DIR/iris/iris.img"

qemu-img create "$IMAGE" "$DISK_SIZE"

parted -s -- "$IMAGE" \
    mklabel gpt \
    mkpart ESP fat32 2048s 100% \
    set 1 esp on

"$IROS_LIMINE_DIR"/limine-deploy "$IMAGE"

cleanup() {
    sync || true
    umount "$IROS_BUILD_DIR/mnt" || true
    losetup -d "$LOOP_DEV" || true
}
trap cleanup EXIT

LOOP_DEV=$(losetup --partscan -f "$IMAGE" --show)

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

sudo mkfs.fat -F 32 "$LOOP_DEV"p1

mkdir -p "$IROS_BUILD_DIR/mnt"
mount "$LOOP_DEV"p1 "$IROS_BUILD_DIR/mnt"

sudo mkdir -p "$IROS_BUILD_DIR"/mnt/EFI/BOOT
sudo cp "$IROS_BUILD_DIR"/iris/iris "$IROS_BUILD_DIR"/iris/test_userspace "$IROS_ROOT/iris/boot/limine.cfg" "$IROS_LIMINE_DIR"/limine.sys "$IROS_BUILD_DIR"/mnt
sudo cp "$IROS_LIMINE_DIR"/BOOTX64.EFI "$IROS_BUILD_DIR"/mnt/EFI/BOOT

chmod 777 "$IMAGE"
