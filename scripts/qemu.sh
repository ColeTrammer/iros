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

# In VS Code remote containers, /dev/kvm is not accessible to this `iris` user. This prevents
# the usage of KVM, which is a signiciant speed reduce. This can however be easily fixed.
if [ "$REMOTE_CONTAINERS" = "true" ]; then
    [ ! -e "/dev/kvm" ] || sudo chmod 666 /dev/kvm
fi

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

PORT_FORWARD=""
if [ "$IROS_PORT_FORWARDING" ]; then
    PORT_FORWARD=",hostfwd=udp:127.0.0.1:8888-10.0.2.15:8888,hostfwd=tcp:127.0.0.1:8823-10.0.2.15:8823"
fi

NETWORK="-netdev user,id=netw -device e1000,netdev=netw$PORT_FORWARD"
if [ "$IROS_DISABLE_NETWORKING" ]; then
    NETWORK="-nic none"
fi
if [ "$IROS_NETWORK_CAPTURE" ]; then
    NETWORK="$NETWORK -object filter-dump,id=f1,netdev=netw,file=$IROS_NETWORK_CAPTURE"
fi

set -x
qemu-system-$ARCH \
    $KERNEL_ARG \
    $CDROM \
    $RUN_SMP \
    $ENABLE_KVM \
    $GRAPHICS \
    $SERIAL \
    $HARDDRIVE \
    $BOOT \
    $NETWORK \
    -d guest_errors \
    -no-reboot \
