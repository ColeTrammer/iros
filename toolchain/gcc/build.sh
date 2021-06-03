#!/bin/sh

ROOT="${ROOT:-$PWD/../..}"
TARGET=`$ROOT/default-host.sh`
VERSION="11.1.0"
DOWNLOAD_URL="http://ftp.gnu.org/gnu/gcc/gcc-$VERSION/gcc-$VERSION.tar.gz"
DOWNLOAD_DEST=gcc.tar.gz
SRC="gcc-$VERSION"
BUILD_DIR="build-gcc"

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
    git apply ../gcc.patch
    git apply ../gcc-run-autoconf.patch
    cd ..
fi

if [ ! -d "$ROOT/sysroot/usr/include" ];
then
    echo -n "Installing libc headers... "
    mkdir -p "$ROOT/sysroot/usr"
    cp -R -p "$ROOT/libs/libc/include" "$ROOT/sysroot/usr"
    echo "Done"
fi

if [ ! -d "$BUILD_DIR" ]; then
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    export gcc_cv_initfini_array=yes
    "../$SRC/configure" \
        --target="$TARGET" \
        --prefix="$ROOT/toolchain/cross" \
        --disable-nls \
        --enable-shared \
        --with-sysroot="$ROOT/sysroot" \
        --enable-languages=c,c++ \
        --enable-threads=posix \
        --with-build-time-tools=$ROOT/toolchain/cross/bin
    cd ..
fi

cd "$BUILD_DIR"
make all-gcc -j5
make all-target-libgcc -j5
make install-gcc install-target-libgcc 

cp "$ROOT/toolchain/cross/$TARGET/lib/libgcc_s.so.*" -t "$ROOT/base/usr/lib"
cd ..
