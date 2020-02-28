#!/bin/sh

set -e

# Variables
export ROOT="$PWD/../.."
export TARGET=`$ROOT/default-host.sh`

if [ ! -e gcc-9.2.0 ]; then
    # Download tar.gz
    curl http://ftp.gnu.org/gnu/gcc/gcc-9.2.0/gcc-9.2.0.tar.gz --output gcc-os_2-9.2.0.tar.gz

    # Extract contents
    tar -xzvf gcc-os_2-9.2.0.tar.gz

    # Apply patch
    cd gcc-9.2.0
    git init
    git apply ../gcc-os_2-9.2.0.patch
    cd ..

    # Run autoconf
    cd gcc-9.2.0
    cd libstdc++-v3
    autoconf
    cd ../..
fi

if [ $ONLY_DOWNLOAD_AND_EXTRACT ]; then 
    exit
fi

# Install headers for correct build
cd ../..
make install-headers
cd toolchain/gcc-9.2.0

# Build
mkdir -p build-gcc
cd build-gcc
../gcc-9.2.0/configure --target=$TARGET --prefix=$ROOT/toolchain/cross --disable-nls --disable-lto --with-sysroot=$ROOT/sysroot --enable-languages=c,c++ --with-build-time-tools=$ROOT/toolchain/cross/bin
make all-gcc -j5
make all-target-libgcc CFLAGS_FOR_TARGET='-g -O2 -mcmodel=large -mno-red-zone' -j5

# Install
mkdir -p ../../cross
make install-gcc install-target-libgcc 

# Build libc for libstdc++-v3
cd ../../../
make prepare-build install-headers native
cd libs/libpthread
make install
cd ../libc
make install
cd ../../toolchain/gcc-9.2.0/build-gcc

make all-target-libstdc++-v3 -j5
make install-target-libstdc++-v3

cd ..
