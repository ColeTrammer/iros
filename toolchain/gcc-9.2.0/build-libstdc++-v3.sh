#!/bin/sh

set -e

# Variables
export ROOT="${ROOT:-$PWD/../..}"
export TARGET=`$ROOT/default-host.sh`

unset CC
unset CXX
unset AS

die() {
    echo "$@" && exit 1
}

[ "$OS_2_BUILD_DIR" ] || die '$OS_2_BUILD_DIR must be set.' 

cmake --build "$OS_2_BUILD_DIR" --target install-libc

cd build-gcc

make all-target-libstdc++-v3 -j5
make install-target-libstdc++-v3

cd "$TARGET/libitm"
make -j5
make install
cd ../..

cp "$ROOT/toolchain/cross/x86_64-os_2/lib/libstdc++.so.6.0.27" "$ROOT/base/usr/lib"
cp "$ROOT/toolchain/cross/x86_64-os_2/lib/libitm.so.1.0.0" "$ROOT/base/usr/lib"

cd ..
