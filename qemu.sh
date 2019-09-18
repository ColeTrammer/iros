#!/bin/sh
# Exits script if make fails
set -e

# Finds HOST and ARCH by defaults if not already set
export HOST=${HOST:-$(./default-host.sh)}
export ARCH=$(./target-triplet-to-arch.sh $HOST)

# Calls make to build iso
make

set +e

# If debug flag is set, calls qemu with full debug, else only prints errors
if echo "$1" | grep -Eq '[-][-]debug'; then
    x-terminal-emulator -e "qemu-system-$ARCH -cdrom os_2.iso -d cpu_reset,guest_errors,int -no-reboot -no-shutdown -monitor stdio -s -S -serial file:debug.log" &
    x-terminal-emulator -e "gdb"
else
    qemu-system-$ARCH -cdrom os_2.iso -d cpu_reset,guest_errors -serial stdio -hda ext2_test.img -boot d
fi