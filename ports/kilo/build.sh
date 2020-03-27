#!/bin/sh

export PORT_NAME=kilo
export SRC_DIR=kilo
export BUILD_DIR=kilo

download() {
    git clone https://github.com/antirez/kilo.git
}

patch() {
    git apply ../kilo.patch
}

install() {
    mkdir -p $ROOT/sysroot/usr/bin
    cp kilo $ROOT/sysroot/usr/bin
}

. ../.build_include.sh