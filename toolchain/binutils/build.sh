#!/bin/sh

set -e

ROOT="${ROOT:-$PWD/../..}"
TARGET="${IROS_TARGET:-`$ROOT/scripts/default-host.sh`}"
VERSION="2.36.1"
DOWNLOAD_URL="http://ftp.gnu.org/gnu/binutils/binutils-$VERSION.tar.gz"
DOWNLOAD_DEST=binutils.tar.gz
SRC="binutils-$VERSION"
BUILD_DIR="build-binutils-$TARGET"
INSTALL_PREFIX="${INSTALL_PREFIX:-$ROOT/toolchain/cross}"

if [ ! -e $DOWNLOAD_DEST ]; then
    curl -L "$DOWNLOAD_URL" -o "$DOWNLOAD_DEST" -4 --retry 5
fi

if [ ! -d "$SRC" ]; then
    tar -xzf "$DOWNLOAD_DEST"
    
    cd "$SRC"
    git init
    git apply ../binutils.patch
    git apply ../binutils-run-autoconf.patch
    cd ..
fi

if [ ! -d "$BUILD_DIR" ]; then
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    "../$SRC/configure" \
        --target="$TARGET" \
        --prefix="$INSTALL_PREFIX" \
        --disable-nls \
        --with-sysroot="$SYSROOT" \
        --disable-werror \
        --enable-shared
    cd ..
fi

cd "$BUILD_DIR"
make -j5
make install
cd ..
