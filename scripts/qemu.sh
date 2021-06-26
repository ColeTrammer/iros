#!/bin/sh

set -e

die() {
    echo $1 && exit 1
}

ARCH=${ARCH:-x86_64}

[ -e os_2.img ] || die 'os_2.img not found - try running `sudo ./makeimg.sh'"'"
[ -e os_2.iso ] || die 'os_2.iso not found - try making target `os_2.iso'"'"

ENABLE_KVM=""
if [ -e /dev/kvm ] && [ -r /dev/kvm ] && [ -w /dev/kvm ]; then
    ENABLE_KVM="-enable-kvm"
fi

qemu-system-$ARCH \
    -cdrom os_2.iso \
    -d guest_errors \
    -serial stdio \
    ${ENABLE_KVM} \
    -device VGA,vgamem_mb=64 \
    -drive file=os_2.img,format=raw,index=0,media=disk \
    -boot d \
    -no-reboot \
    -no-shutdown \
    -netdev user,id=breh,hostfwd=udp:127.0.0.1:8888-10.0.2.15:8888,hostfwd=tcp:127.0.0.1:8823-10.0.2.15:8823 \
    -object filter-dump,id=f1,netdev=breh,file=e1000.pcap \
    -device e1000,netdev=breh
