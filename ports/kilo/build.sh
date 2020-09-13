#!/bin/sh

export PORT_NAME=kilo
export SRC_DIR=kilo
export BUILD_DIR=kilo
export CC=x86_64-os_2-gcc

download() {
    git clone https://github.com/antirez/kilo.git
}

install() {
    mkdir -p $ROOT/sysroot/usr/bin
    cp kilo $ROOT/sysroot/usr/bin
}

. ../.build_include.sh
