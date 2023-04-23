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

DEFAULT_DISK_SIZE=64m
DISK_SIZE="${IROS_DISK_SIZE:-$DEFAULT_DISK_SIZE}"

IMAGE="${IROS_IMAGE:-$IROS_BUILD_DIR/iris/iris.img}"
LIMINE_CFG="${IROS_LIMINE_CFG:-$IROS_ROOT/iris/boot/limine.cfg}"
INITRD="${IROS_INITRD:-$IROS_BUILD_DIR/initrd/initrd.bin}"

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

    if [ "$REMOTE_CONTAINERS" = true ] && [ "$HACK_CREATED_LOOP0" = true ]; then
        # HACK to prevent build from failing in new kernels. Delete loop0 so that
        # is will be re-used in future runs of this script. This is needed to workaround
        # strange docker/linux issues.
        rm -f /dev/loop0*
    fi
}
trap cleanup EXIT

if [ "$REMOTE_CONTAINERS" = 'true' ]; then
    # HACK to prevent build from failing in new kernels. For some reason, the loopback device
    # wont show up. On a system with existing loopback devices, like Ubuntu with snap's, this
    # shouldn't be needed. But for some reason, on arch linux with new kernels, losetup fails.
    # This could be because the dev container has an older version of losetup than required, but
    # I haven't investigated this yet.
    if ! [ -e "/dev/loop0" ]; then
        mknod /dev/loop0 b 7 0;
        export HACK_CREATED_LOOP0=true;
    fi
fi

if [ "$GITHUB_RUN_ID" ]; then
    echo "Attempting HACK, running on GitHub actions."
    for file in /dev/loop*; do
        if ! [ "$file" = "/dev/loop-control" ]; then
        echo "Trying to unlink: $file"
            losetup -d "$file" || true
            rm -f "file"
        fi
    done
    sleep 1
else
    echo "Not using HACK, not running on GitHub actions."
fi

LOOP_DEV=$(losetup --partscan -f "$IMAGE" --show)

if [ "$GITHUB_RUN_ID" ]; then
    sleep 1
    echo "HACK: running partprobe."
    partprobe
    sleep 1
fi

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
sudo objcopy -g "$IROS_BUILD_DIR"/iris/iris "$IROS_BUILD_DIR"/mnt/iris
sudo cp "$INITRD" "$LIMINE_CFG" "$IROS_LIMINE_DIR"/limine.sys "$IROS_BUILD_DIR"/mnt
sudo cp "$IROS_LIMINE_DIR"/BOOTX64.EFI "$IROS_BUILD_DIR"/mnt/EFI/BOOT

chmod 777 "$IMAGE"
