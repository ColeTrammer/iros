#!/bin/sh

set -e

die() {
    echo $1 && exit 1
}

ARCH=${IROS_ARCH:-x86_64}

ROOT=${ROOT:-`realpath ..`}
BUILD_DIR="${IROS_BUILD_DIR:-$ROOT/build_$ARCH}"
IMAGE="${IROS_IMAGE:-$BUILD_DIR/iros/iros.img}"
KERNEL="${IROS_KERNEL:-$BUILD_DIR/iros/kernel/kernel}"
INITRD="${IROS_INITRD:-$BUILD_DIR/iros/sysroot/boot/initrd.bin}"
ISO="${IROS_ISO:-$BUILD_DIR/iros/iros.iso}"
RUN="${IROS_RUN:-kernel}"

[ "$IROS_DISABLE_HARDDRIVE" ] || [ -e "$IMAGE" ] || die 'iros.img not found - try running `ninja image'"'"
[ "$RUN" != iso ] || [ -e "$ISO" ] || die 'iros.iso not found - try running `ninja iso'"'"
[ "$RUN" != kernel ] || [ -e "$KERNEL" ] || die 'kernel not found - try running `ninja'"'"
[ "$RUN" != kernel ] || [ -e "$INITRD" ] || die 'initrd not found - try running `ninja install'"'"

ENABLE_KVM=""
if ! [ "$IROS_DISABLE_KVM" ] && [ -e /dev/kvm ] && [ -r /dev/kvm ] && [ -w /dev/kvm ]; then
    ENABLE_KVM="-enable-kvm"
fi

RUN_SMP=""
if [ "$IROS_RUN_SMP" ]; then
    RUN_SMP="-smp $IROS_RUN_SMP"
fi

SERIAL="-serial stdio"
if [ "$IROS_DISABLE_GRAPHICS" ]; then
    GRAPHICS="-device VGA,vgamem_mb=64 -nographic"
    SERIAL=""
else
    GRAPHICS="-device VGA,vgamem_mb=64"
fi

HARDDRIVE=""
if [ ! "$IROS_DISABLE_HARDDRIVE" ]; then
    HARDDRIVE="-drive file="$IMAGE",format=raw,index=0,media=disk"
fi

CDROM=""
BOOT=""
KERNEL_ARG=""
if [ "$RUN" = iso ]; then
    CDROM="-cdrom $ISO"
    BOOT="-boot d"
elif [ "$RUN" = kernel ]; then
    KERNEL_ARG="-kernel $KERNEL -initrd $INITRD"
    if [ "$IROS_CMDLINE" ]; then
        KERNEL_ARG="$KERNEL_ARG -append $IROS_CMDLINE"
    fi
elif [ "$RUN" = harddrive ]; then
    :;
else
    die 'Unkown value of IROS_RUN: `'"$RUN'"
fi

if [ "$ARCH" = "i686" ]; then
    ARCH="i386"
fi

qemu-system-$ARCH \
    $KERNEL_ARG \
    $CDROM \
    -d guest_errors \
    $RUN_SMP \
    $ENABLE_KVM \
    $GRAPHICS \
    $SERIAL \
    $HARDDRIVE \
    $BOOT \
    -no-reboot \
    -netdev user,id=breh,hostfwd=udp:127.0.0.1:8888-10.0.2.15:8888,hostfwd=tcp:127.0.0.1:8823-10.0.2.15:8823 \
    -object filter-dump,id=f1,netdev=breh,file=e1000.pcap \
    -device e1000,netdev=breh
