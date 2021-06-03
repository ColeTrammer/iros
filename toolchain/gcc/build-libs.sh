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


cd "$BUILD_DIR"
cmake --build "$OS_2_BUILD_DIR" --target libc
mkdir -p "$ROOT/sysroot/lib"
cp "$OS_2_BUILD_DIR/libs/libc/libc.so" "$ROOT/sysroot/lib"

make all-target-libgcc -j5
make install-target-libgcc
cp -p $ROOT/toolchain/cross/$TARGET/lib/libgcc_s.so* -t "$ROOT/base/usr/lib"

cmake --build "$OS_2_BUILD_DIR" --target bootstrap-core-libs

make all-target-libstdc++-v3 -j5
make install-target-libstdc++-v3
cp -p $ROOT/toolchain/cross/x86_64-os_2/lib/libstdc++.so* -t "$ROOT/base/usr/lib"
cd ..
