#!/bin/sh
set -e

export HOST=${HOST:-$(./default-host.sh)}
export ARCH=$(./target-triplet-to-arch.sh $HOST)

make

if echo "$1" | grep -Eq '[-]debug'; then
    qemu-system-$ARCH -cdrom os_2.iso -d cpu
else
    qemu-system-$ARCH -cdrom os_2.iso -d guest_errors
fi