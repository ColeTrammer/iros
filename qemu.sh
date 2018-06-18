#!/bin/sh
# Exits script if make fails
set -e

# Finds HOST and ARCH by defaults if not already set
export HOST=${HOST:-$(./default-host.sh)}
export ARCH=$(./target-triplet-to-arch.sh $HOST)

# Calls make to build iso
make

# If debug flag is set, calls qemu with full debug, else only prints errors
if echo "$1" | grep -Eq '[-][-]debug'; then
    qemu-system-$ARCH -cdrom os_2.iso -d cpu_reset,guest_errors,int -no-reboot -no-shutdown -monitor stdio
else
    qemu-system-$ARCH -cdrom os_2.iso -d guest_errors
fi