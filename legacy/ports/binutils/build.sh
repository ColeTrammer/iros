#!/bin/sh

export PORT_NAME=binutils
export SRC_DIR='../../toolchain/binutils-2.34/binutils-os_2-2.34'
export BUILD_DIR='build-binutils'
export INSTALL_COMMAND=${INSTALL_COMMAND:-'install-strip'}

download() {
    :
}

configure() {
    $ROOT/toolchain/binutils-2.34/binutils-os_2-2.34/configure \
        --host="$HOST" \
        --disable-nls \
        --prefix=/usr \
        --target="$HOST" \
        --with-sysroot=/ \
        --with-build-sysroot="$ROOT/sysroot" \
        --disable-werror \
        --disable-gdb \
        --enable-shared \
        --enable-host-shared \
        --with-system-zlib
}

. ../.build_include.sh
