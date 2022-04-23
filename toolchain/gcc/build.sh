#!/bin/sh

set -e

ROOT="${ROOT:-$PWD/../..}"
TARGET="${IROS_TARGET:-`$ROOT/scripts/default-host.sh`}"
VERSION="11.1.0"
DOWNLOAD_URL="http://ftp.gnu.org/gnu/gcc/gcc-$VERSION/gcc-$VERSION.tar.gz"
DOWNLOAD_DEST=gcc.tar.gz
SRC="gcc-$VERSION"
BUILD_DIR="build-gcc-$TARGET"
INSTALL_PREFIX="${INSTALL_PREFIX:-$ROOT/toolchain/cross}"

if [ ! -e $DOWNLOAD_DEST ]; then
    curl -L "$DOWNLOAD_URL" -o "$DOWNLOAD_DEST" -4 --retry 5
fi

if [ ! -d "$SRC" ]; then
    tar -xzf "$DOWNLOAD_DEST"
    
    cd "$SRC"
    git init
    git apply ../gcc.patch
    git apply ../gcc-run-autoconf.patch
    cd ..
fi

if [ ! -d "$ROOT/sysroot/usr/include" ];
then
    echo -n "Installing libc headers... "
    mkdir -p "$SYSROOT/usr"
    cp -R -p "$ROOT/libs/libc/include" "$SYSROOT/usr"
    echo "Done"
fi

if [ ! -d "$BUILD_DIR" ]; then
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    "../$SRC/configure" \
        --target="$TARGET" \
        --prefix="$INSTALL_PREFIX" \
        --disable-nls \
        --enable-shared \
        --with-sysroot="$SYSROOT" \
        --enable-languages=c,c++ \
        --enable-threads=posix \
        --with-build-time-tools=$INSTALL_PREFIX/bin
    cd ..
fi

cd "$BUILD_DIR"
make all-gcc -j5
make install-gcc
cd ..
