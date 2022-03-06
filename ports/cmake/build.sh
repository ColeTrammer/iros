#!/bin/sh

PORT_NAME=cmake
SRC_DIR='cmake'
BUILD_DIR='build-cmake'
VERSION='3.20.3'

download() {
    :
}

configure() {
    # Export necessary build flags/info
    export CC='x86_64-os_2-gcc'
    export CXX='x86_64-os_2-g++'
    export CFLAGS='-O2 -fno-omit-frame-pointer'
    export CXXFLAGS='-O2 -fno-omit-frame-pointer'

    # Prevent cmake from trying a use a cross compiled make
    if [ -e "$ROOT/sysroot/usr/bin/make" ]; then
        mv "$ROOT/sysroot/usr/bin/make" _make
    fi

    # Run twice because of TryRunResults.cmake
    cmake -DCMAKE_SYSTEM_NAME=OS_2 -DCMAKE_SYSROOT="$ROOT/sysroot" -DCMAKE_USE_SYSTEM_CURL=ON -S "$ROOT/toolchain/cmake/cmake-$VERSION"
    cmake -DCMAKE_SYSTEM_NAME=OS_2 -DCMAKE_SYSROOT="$ROOT/sysroot" -DCMAKE_USE_SYSTEM_CURL=ON -S "$ROOT/toolchain/cmake/cmake-$VERSION"

    if [ -e "_make" ]; then
        mv _make "$ROOT/sysroot/usr/bin/make"
    fi
}

install() {
    cmake -DCMAKE_INSTALL_PREFIX="$ROOT/sysroot/usr" -P cmake_install.cmake
}

. ../.build_include.sh
