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

qemu-system-"$IROS_ARCH" \
    $ENABLE_KVM \
    -drive file="$IROS_IMAGE",format=raw,index=0,media=disk \
    -cpu max \
    -serial stdio \
    -no-reboot \
    -d guest_errors \
    -display none \
    -device isa-debug-exit,iobase=0xf4,iosize=0x04
