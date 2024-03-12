#!/bin/sh

if [ ! "$IROS_ARCH" ] || [ ! "$IROS_IMAGE" ]; then
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
    echo "Running with KVM enabled..."
else
    echo "Running with KVM disabled..."
fi

LOG="-d guest_errors,cpu_reset"
if [ "$IROS_DEBUG" ]; then
    DEBUG="-s -S -no-reboot -no-shutdown"
    LOG="$LOG"
fi
SERIAL="-serial mon:stdio"

if [ ! "$IROS_NO_SMP" ]; then
    if [ $(nproc) -gt 4 ]; then
        SMP='-smp 4'
    else
        SMP="-smp $(nproc)"
    fi
fi

if [ "$IRIS_MEMORY" ]; then
    MEMORY="-m $IRIS_MEMORY"
else
    # NOTE: the default memory size for Qemu is 128M, but we need more to successfully execute the test suite. This is
    # almost entirely because both the kernel and userspace use naive memory allocation strategies (they don't ever free
    # or reuse memory). When this gets fixed, we can reduce this to 128M.
    MEMORY="-m 256M"
fi

qemu-system-"$IROS_ARCH" \
    $ENABLE_KVM \
    $DEBUG \
    $SERIAL \
    $SMP \
    $MEMORY \
    $LOG \
    -drive file="$IROS_IMAGE",format=raw,index=0,media=disk \
    -cpu max \
    -no-reboot \
    -display none \
    -device isa-debug-exit,iobase=0xf4,iosize=0x04
