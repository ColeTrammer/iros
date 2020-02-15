#!/bin/sh

set -e

# Variables
export ROOT="$PWD/../.."
export HOST=`$ROOT/default-host.sh`

# Install headers for correct build
cd ../..
make install-headers
cd ports/binutils

# Build
mkdir -p build-binutils
cd build-binutils
../../../toolchain/binutils-2.34/binutils-2.34 --host=$HOST --disable-nls --prefix=/usr --target=$HOST --with-sysroot=/ --with-build-sysroot=$ROOt/sysroot --disable-werror --disable-gdb

make clean
make all -j5
make install DESTDIR=$ROOT/sysroot -j5
cd ..
