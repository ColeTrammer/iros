#!/bin/sh

export PORT_NAME=doom
export SRC_DIR='doom'
export BUILD_DIR="$SRC_DIR/doomgeneric"

download() {
    git clone https://github.com/ozkl/doomgeneric "$SRC_DIR"
    cd "$SRC_DIR"
    git reset --hard 580e19a7b8e105e0a3f56f4248b8e15f1b264e1e
    cd ..
}

patch() {
    git apply ../doom.patch
}

build() {
    make -f Makefile.iros -j5
}

install() {
    make install -f Makefile.iros DESTDIR="$SYSROOT"
}

. ../.build_include.sh
