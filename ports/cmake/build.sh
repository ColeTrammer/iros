#!/bin/sh

PORT_NAME=cmake
SRC_DIR='cmake'
BUILD_DIR='cmake/build-cmake'

download() {
    git clone https://gitlab.kitware.com/cmake/cmake --depth=1
}

patch() {
    git apply ../cmake.patch
}

configure() {
    # Bootstrap CMake so that it thinks the Os_2 system name actually exists.
    mkdir -p ../build-temp
    cd ../build-temp
    export CC=gcc
    export CXX=g++
    unset CFLAGS
    unset CXXFLAGS
    ../bootstrap && make -j5
    cd "../../$BUILD_DIR"

    # Export the path with the bootstapped cmake
    export PATH="$ROOT/ports/cmake/$SRC_DIR/build-temp/bin:$PATH"
    
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
    cmake -DCMAKE_SYSTEM_NAME=Os_2 -DCMAKE_SYSROOT="$ROOT/sysroot" -S ..
    cmake -DCMAKE_SYSTEM_NAME=Os_2 -DCMAKE_SYSROOT="$ROOT/sysroot" -S ..
    if [ -e "_make" ]; then
        mv _make "$ROOT/sysroot/usr/bin/make"
    fi
}

install() {
    cmake -DCMAKE_INSTALL_PREFIX="$ROOT/sysroot/usr" -P cmake_install.cmake
}

. ../.build_include.sh
