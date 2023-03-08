#!/bin/sh

set -e

VERSION=12.2.0
PATCH_DIR=$(realpath $(dirname -- "$0"))
PROJECT_ROOT=$(realpath "$PATCH_DIR"/../../..)
PREFIX="${IROS_PREFIX:-$PROJECT_ROOT/cross}"
SYSROOT="$PROJECT_ROOT"/build/x86_64/sysroot
NPROC=$(nproc)

git clone "https://gcc.gnu.org/git/gcc.git" --depth=1 --branch "releases/gcc-$VERSION" src

cd src
git apply $PATCH_DIR/*.patch
cd ..

mkdir -p build
cd build

# Install libccpp headers
mkdir -p "$SYSROOT"/usr/include
cp -r "$PROJECT_ROOT"/libs/ccpp/include/* "$SYSROOT"/usr/include

# Disable debug symbols.
export CFLAGS="-g0 -O3"
export CXXFLAGS="-g0 -O3"

../src/configure \
    --disable-nls \
    --target=x86_64-iros \
    --prefix="$PREFIX" \
    --with-sysroot="$SYSROOT" \
    --with-build-time-tools="$PREFIX/bin" \
    --enable-languages=c,c++

make all-gcc -j"$NPROC"
make install-gcc -j"$NPROC"

(
    # Build and install Iros libccpp
    cd "$PROJECT_ROOT"
    cmake --preset iros_x86_64 -DIROS_NeverBuildDocs=ON
    cmake --build --preset iros_x86_64 --target ccpp
    cmake --build --preset iros_x86_64 --target libs/ccpp/install
)

make all-target-libgcc -j"$NPROC"
make install-target-libgcc -j"$NPROC"
