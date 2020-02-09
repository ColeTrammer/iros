#!/bin/sh

set -e

# Variables
export ROOT="$PWD/../.."
export TARGET=`$ROOT/default-host.sh`

if [ ! -e binutils-os_2-2.34 ]; then
    # Download tar.gz
    curl https://ftp.gnu.org/gnu/binutils/binutils-2.34.tar.gz --output binutils-os_2-2.34.tar.gz

    # Extract contents
    tar -xzvf binutils-os_2-2.34.tar.gz

    # Apply patch
    mv binutils-2.34 binutils-os_2-2.34
    cd binutils-os_2-2.34
    git init
    git apply ../binutils-os_2-2.34.patch
    cd ..
fi

# Install headers for correct build
cd ../..
make install-headers
cd toolchain/binutils-2.34

# Build
mkdir -p build-binutils
cd build-binutils
../binutils-os_2-2.34/configure --target=$TARGET --prefix=$ROOT/toolchain/cross --disable-nls --with-sysroot=$ROOT/sysroot --disable-werror
make -j5

# Install
mkdir -p $ROOT/toolchain/cross
make install
cd ..
