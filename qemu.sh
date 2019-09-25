#!/bin/sh
# Exits script if make fails
set -e

# Finds HOST and ARCH by defaults if not already set
export HOST=${HOST:-$(./default-host.sh)}
export ARCH=$(./target-triplet-to-arch.sh $HOST)

# If debug flag is set, calls qemu with full debug, else only prints errors
if echo "$1" | grep -Eq '[-][-]debug'; then
    export DEFINES="-DKERNEL_NO_DEBUG_COLORS"
    make clean all

    set +e

    x-terminal-emulator -e "qemu-system-$ARCH -cdrom os_2.iso -d cpu_reset,guest_errors,int -no-reboot -no-shutdown -monitor stdio -s -S -serial file:debug.log" &
    x-terminal-emulator -e "gdb"
else
    export DEFINES=""

    # Calls make to build iso
    make

    set +e

    qemu-system-$ARCH -cdrom os_2.iso -d cpu_reset,guest_errors -serial stdio -hda ext2_test.img -boot d
fi