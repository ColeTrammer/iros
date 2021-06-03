#!/bin/sh

set -e

ROOT="${ROOT:-$PWD/../..}"
VERSION="3.20.3"
DOWNLOAD_URL="https://github.com/Kitware/CMake/releases/download/v$VERSION/cmake-$VERSION.tar.gz"
DOWNLOAD_DEST=cmake.tar.gz
SRC="cmake-$VERSION"

unset CC
unset CXX
unset AS

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
    ./bootstrap --prefix="$ROOT/toolchain/cross" --parallel=5
    make -j5
    make install
    cd ..
fi
