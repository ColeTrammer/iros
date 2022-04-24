#!/bin/sh

set -e

ROOT="${ROOT:-$PWD/../..}"
VERSION="3.20.3"
DOWNLOAD_URL="https://github.com/Kitware/CMake/releases/download/v$VERSION/cmake-$VERSION.tar.gz"
DOWNLOAD_DEST=cmake.tar.gz
SRC="cmake-$VERSION"
INSTALL_PREFIX="${INSTALL_PREFIX:-$ROOT/toolchain/cross}"
NPROC="${NPROC:-`nproc`}"

exists() {
    $1 --version >/dev/null 2>&1
}

if [ ! -e $DOWNLOAD_DEST ]; then
    curl -L "$DOWNLOAD_URL" -o "$DOWNLOAD_DEST" -4 --retry 5
fi

if [ ! -d "$SRC" ]; then
    tar -xzf "$DOWNLOAD_DEST"
    
    cd "$SRC"
    git init
    git apply ../cmake.patch
    cd ..
fi

if [ ! -e "$ROOT/toolchain/cross/bin/cmake" ]; then
    cd "$SRC"
    if exists cmake; then
        cmake -S . -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX"
        make -j5
        make install
    else
        ./bootstrap --prefix="$INSTALL_PREFIX" "--parallel=$NPROC"
        make "-j$NPROC"
        make install
    fi
    cd ..
fi
