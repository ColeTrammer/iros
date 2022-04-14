#!/bin/sh

set -e

ROOT="${ROOT:-$PWD/../..}"
TARGET="${IROS_TARGET:-`$ROOT/scripts/default-host.sh`}"
BUILD_DIR="build-gcc-$TARGET"

die() {
    echo "$@" && exit 1
}

[ $IROS_BUILD ] || die "IROS_BUILD must be defined."

cmake --build "$ROOT/$IROS_BUILD" --target iros-bootstrap-core-libs

cd "$BUILD_DIR"
make all-target-libgcc -j5
make install-target-libgcc
cp -pd $ROOT/toolchain/cross/$TARGET/lib/libgcc_s.so* -t "$SYSROOT/usr/lib"

make all-target-libstdc++-v3 -j5
make install-target-libstdc++-v3
cp -pd $ROOT/toolchain/cross/x86_64-iros/lib/libstdc++.so* -t "$SYSROOT/usr/lib"
cd ..
