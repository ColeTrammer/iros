#!/bin/sh

export PORT_NAME=binutils
export SRC_DIR='../../toolchain/binutils-2.34/binutils-os_2-2.34'
export BUILD_DIR='build-binutils'

download() {
    :
}

patch() {
    :
}

configure() {
    $ROOT/toolchain/binutils-2.34/binutils-os_2-2.34/configure --host=$HOST --disable-nls --prefix=/usr --target=$HOST --with-sysroot=/ --with-build-sysroot=$ROOt/sysroot --disable-werror --disable-gdb
}

clean() {
    make clean
}

build() {
    make -j5
}

install() {
    make install-strip DESTDIR=$ROOT/sysroot -j5
}

. ../.build_include.sh