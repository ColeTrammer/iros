#!/bin/sh

set -e

die() {
    echo $1 && exit 1
}

ARCH=${ARCH:-x86_64}

[ -e os_2.img ] || die 'os_2.img not found - try running `sudo ./makeimg.sh'"'"
[ -e os_2.iso ] || die 'os_2.iso not found - try making target `os_2.iso'"'"

qemu-system-$ARCH \
    -cdrom os_2.iso \
    -d cpu_reset,guest_errors \
    -serial stdio \
    -smp 2 \
    -device VGA,vgamem_mb=64 \
    -hda os_2.img \
    -boot d \
    -no-reboot \
    -no-shutdown \
    -object filter-dump,id=hue,netdev=breh,file=e1000.pcap \
    -netdev user,id=breh,hostfwd=tcp:127.0.0.1:8888-10.0.2.15:8888,hostfwd=tcp:127.0.0.1:8823-10.0.2.15:23 \
    -device e1000,netdev=breh
