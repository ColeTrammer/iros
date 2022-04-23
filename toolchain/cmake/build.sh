#!/bin/sh

set -e

ROOT="${ROOT:-$PWD/../..}"
VERSION="3.20.3"
DOWNLOAD_URL="https://github.com/Kitware/CMake/releases/download/v$VERSION/cmake-$VERSION.tar.gz"
DOWNLOAD_DEST=cmake.tar.gz
SRC="cmake-$VERSION"

exists() {
    $1 --version >/dev/null 2>&1
}

if [ ! -e $DOWNLOAD_DEST ]; then
    curl -L "$DOWNLOAD_URL" -o "$DOWNLOAD_DEST"
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
        cmake -S .
        make -j5
        make install
    else
        ./bootstrap --prefix="$ROOT/toolchain/cross" --parallel=5
        make -j5
        make install
    fi
    cd ..
fi
