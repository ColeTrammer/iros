#!/bin/sh

set -e

VERSION=2_40
PATCH_DIR=$(realpath $(dirname -- "$0"))
PROJECT_ROOT=$(realpath "$PATCH_DIR"/../../..)
PREFIX="${IROS_PREFIX:-$PROJECT_ROOT/cross}"
SYSROOT="$PROJECT_ROOT"/build/x86_64/sysroot
NPROC=$(nproc)

git clone "https://sourceware.org/git/binutils-gdb.git" --depth=1 --branch "binutils-$VERSION" src

cd src
git apply $PATCH_DIR/*.patch
cd ..

mkdir -p build
cd build

# Disable debug symbols
export CFLAGS="-g0 -O3"
export CXXFLAGS="-g0 -O3"

../src/configure \
    --disable-nls \
    --disable-werror \
    --target=x86_64-iros \
    --prefix="$PREFIX" \
    --with-sysroot="$SYSROOT" \
    --disable-gdb

mkdir -p gas/doc

make -j"$NPROC"
make install -j"$NPROC"
