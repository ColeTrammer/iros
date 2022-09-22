#!/bin/sh

set -e

if [ "$(id -u)" != 0 ];
then
    echo "This script must be ran as root."
    exit 1
fi

if [ ! "$BUILD_DIR" ] || [ ! "$ROOT" ] || [ ! "$LIMINE_DIR" ];
then
    echo "BUILD_DIR, ROOT, LIMINE_DIR all must be set."
    exit 1
fi

echo "$LIMINE_DIR"

DEFAULT_DISK_SIZE=64m
DISK_SIZE="${IROS_DISK_SIZE:-$DEFAULT_DISK_SIZE}"

IMAGE="$BUILD_DIR/iris.img"

qemu-img create "$IMAGE" "$DISK_SIZE"

parted -s -- "$IMAGE" \
    mklabel gpt \
    mkpart ESP fat32 2048s 100% \
    set 1 esp on

"$LIMINE_DIR"/limine-deploy "$IMAGE"

cleanup() {
    sync || true
    umount "$BUILD_DIR/mnt" || true
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

mkdir -p "$BUILD_DIR/mnt"
mount "$LOOP_DEV"p1 "$BUILD_DIR/mnt"

sudo mkdir -p "$BUILD_DIR"/mnt/EFI/BOOT
sudo cp "$BUILD_DIR"/iris "$ROOT/iris/boot/limine.cfg" "$LIMINE_DIR"/limine.sys "$BUILD_DIR"/mnt
sudo cp "$LIMINE_DIR"/BOOTX64.EFI "$BUILD_DIR"/mnt/EFI/BOOT

chmod 777 "$IMAGE"
