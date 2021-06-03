#!/bin/sh

set -e

ROOT="${ROOT:-$PWD/../..}"
TARGET=`$ROOT/default-host.sh`
BUILD_DIR="build-gcc"

unset CC
unset CXX
unset AS

die() {
    echo "$@" && exit 1
}

[ "$OS_2_BUILD_DIR" ] || die '$OS_2_BUILD_DIR must be set.' 

cmake --build "$OS_2_BUILD_DIR" --target install-libc

cd "$BUILD_DIR"
make all-target-libstdc++-v3 -j5
make install-target-libstdc++-v3
cp "$ROOT/toolchain/cross/x86_64-os_2/lib/libstdc++.so.*" -t "$ROOT/base/usr/lib"
cd ..
