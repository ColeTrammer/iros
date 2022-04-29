#!/bin/sh

export PORT_NAME=doom
export SRC_DIR='doom'
export BUILD_DIR="$SRC_DIR/doomgeneric"

download() {
    git clone https://github.com/ozkl/doomgeneric --depth=1 "$SRC_DIR"
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
