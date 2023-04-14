#!/bin/sh

if [ ! "$IROS_ARCH" ] || [ ! "$IROS_IMAGE" ];
then
    echo "IROS_ARCH, and IROS_IMAGE all must be set."
    exit 1
fi

# In VS Code remote containers, /dev/kvm is not accessible to this `iris` user. This prevents
# the usage of KVM, which is a signiciant speed reduce. This can however be easily fixed.
if [ "$REMOTE_CONTAINERS" = "true" ]; then
    [ ! -e "/dev/kvm" ] || sudo chmod 666 /dev/kvm
fi

ENABLE_KVM=""
if ! [ "$IROS_DISABLE_KVM" ] && [ -e /dev/kvm ] && [ -r /dev/kvm ] && [ -w /dev/kvm ]; then
    ENABLE_KVM="-enable-kvm"
fi

if [ "$IROS_DEBUG" ]; then
    DEBUG="-s -S -monitor stdio -no-reboot -no-stutdown -d cpu_reset"
else
    SERIAL="-serial stdio"
fi

if [ ! "$IROS_NO_SMP" ]; then
    if [ `nproc` -gt 4 ]; then
        SMP='-smp 4'
    else
        SMP="-smp `nproc`"
    fi
fi

qemu-system-"$IROS_ARCH" \
    $ENABLE_KVM \
    $DEBUG \
    $SERIAL \
    $SMP \
    -drive file="$IROS_IMAGE",format=raw,index=0,media=disk \
    -cpu max \
    -no-reboot \
    -d guest_errors \
    -display none \
    -device isa-debug-exit,iobase=0xf4,iosize=0x04
