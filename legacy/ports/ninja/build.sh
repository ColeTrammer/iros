#!/bin/sh

export PORT_NAME=ninja
export SRC_DIR='ninja'
export BUILD_DIR="$SRC_DIR/build"
export INSTALL_COMMAND=${INSTALL_COMMAND:-'install'}

export CC='x86_64-os_2-gcc'
export CXX='x86_64-os_2-g++'
export CFLAGS='-O2 -fno-omit-frame-pointer'
export CXXFLAGS='-O2 -fno-omit-frame-pointer'

download() {
    git clone https://github.com/ninja-build/ninja.git --depth=1
}

patch() {
    git apply ../ninja.patch
}

configure() {
    cmake -DCMAKE_TOOLCHAIN_FILE="$ROOT/cmake/CMakeToolchain_x86_64.txt" -G Ninja -S ..
}

build() {
    ninja
}

install() {
    DESTDIR="$SYSROOT" ninja install
}

. ../.build_include.sh
