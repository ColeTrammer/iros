#!/bin/sh

set -e

VERSION=12.2.0
PATCH_DIR=$(realpath $(dirname -- "$0"))
PROJECT_ROOT=$(realpath "$PATCH_DIR"/../../..)
PREFIX="$PROJECT_ROOT"/cross
SYSROOT="$PROJECT_ROOT"/build/x86_64/sysroot
NPROC=$(nproc)

# git clone "https://gcc.gnu.org/git/gcc.git" --depth=1 --branch "releases/gcc-$VERSION" src

# cd src
# git am $PATCH_DIR/*.patch
# cd ..

mkdir -p build
cd build

cp -r "$PROJECT_ROOT"/libs/ccpp/include/* "$SYSROOT"/usr/include

../src/configure \
    --disable-nls \
    --target=x86_64-iros \
    --prefix="$PREFIX" \
    --with-sysroot="$SYSROOT" \
    --with-build-time-tools="$PREFIX/bin" \
    --enable-languages=c,c++

make all-gcc -j"$NPROC"
make install-gcc -j"$NPROC"
make all-target-libgcc -j"$NPROC"
make install-target-libgcc -j"$NPROC"