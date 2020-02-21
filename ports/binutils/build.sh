#!/bin/bash

set -e

# Variables
export ROOT="$PWD/../.."
export HOST=`$ROOT/default-host.sh`
PORT_DIR=$ROOT/toolchain/binutils-2.34

export ONLY_DOWNLOAD_AND_EXTRACT=1
pushd $PORT_DIR
./build.sh
popd

# Install headers for correct build
cd ../..
make install-headers
cd ports/binutils
rm $ROOT/sysroot/usr/include/dlfcn.h

# Build
mkdir -p build-binutils
cd build-binutils
$PORT_DIR/binutils-os_2-2.34/configure --host=$HOST --disable-nls --prefix=/usr --target=$HOST --with-sysroot=/ --with-build-sysroot=$ROOt/sysroot --disable-werror --disable-gdb

make clean
make all -j5
make install-strip DESTDIR=$ROOT/sysroot -j5
cd ..
