#!/bin/sh

set -e

ROOT="${ROOT:-$PWD/../..}"
TARGET=`$ROOT/scripts/default-host.sh`
BUILD_DIR="build-gcc"

die() {
    echo "$@" && exit 1
}

cmake --build "$ROOT/build" --target os_2-bootstrap-core-libs

cd "$BUILD_DIR"
make all-target-libgcc -j5
make install-target-libgcc
cp -pd $ROOT/toolchain/cross/$TARGET/lib/libgcc_s.so* -t "$ROOT/sysroot/usr/lib"

make all-target-libstdc++-v3 -j5
make install-target-libstdc++-v3
cp -pd $ROOT/toolchain/cross/x86_64-os_2/lib/libstdc++.so* -t "$ROOT/sysroot/usr/lib"
cd ..
