#!/bin/sh

set -e

ROOT="${ROOT:-$PWD/../..}"
TARGET=`$ROOT/scripts/default-host.sh`
BUILD_DIR="build-gcc"

die() {
    echo "$@" && exit 1
}

cmake --build "$ROOT/build" --target bootstrap-core-libs

make all-target-libgcc -j5
make install-target-libgcc
cp -p $ROOT/toolchain/cross/$TARGET/lib/libgcc_s.so* -t "$ROOT/base/usr/lib"

make all-target-libstdc++-v3 -j5
make install-target-libstdc++-v3
cp -p $ROOT/toolchain/cross/x86_64-os_2/lib/libstdc++.so* -t "$ROOT/base/usr/lib"
cd ..
