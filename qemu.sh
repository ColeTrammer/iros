#!/bin/sh
# Exits script if make fails
set -e

# Finds HOST and ARCH by defaults if not already set
export HOST=${HOST:-$(./default-host.sh)}
export ARCH=$(./target-triplet-to-arch.sh $HOST)

# If debug flag is set, calls qemu with full debug, else only prints errors
if [ "X$1" -eq 'X--debug' ]; then
    export DEFINES="-DKERNEL_NO_DEBUG_COLORS"
    make clean all

    set +e

    x-terminal-emulator -e "qemu-system-$ARCH -cdrom os_2.iso -d cpu_reset,guest_errors,int -hda os_2.img -no-reboot -no-shutdown -monitor stdio -s -S -serial file:debug.log" &
    x-terminal-emulator -e "gdb"
else
    # export DEFINES="-DKERNEL_NO_GRAPHICS -DKERNEL_NO_DEBUG_COLORS -DKERNEL_MALLOC_DEBUG"
    export DEFINES="-DKERNEL_NO_GRAPHICS"
    # export DEFINES=""

    # Calls make to build iso
    make

    set +e

    qemu-system-$ARCH \
        -cdrom os_2.iso \
        -d cpu_reset,guest_errors \
        -serial stdio \
        -device VGA,vgamem_mb=64 \
        -hda os_2.img \
        -boot d \
        -no-reboot \
        -no-shutdown \
        -object filter-dump,id=hue,netdev=breh,file=e1000.pcap \
        -netdev user,id=breh,hostfwd=tcp:127.0.0.1:8888-10.0.2.15:8888,hostfwd=tcp:127.0.0.1:8823-10.0.2.15:23 \
        -device e1000,netdev=breh
fi