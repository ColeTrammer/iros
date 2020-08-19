#!/bin/sh

set -e

# Variables
export ROOT="${ROOT:-$PWD/../..}"
export TARGET=`$ROOT/default-host.sh`

CC=gcc
CXX=g++
AS=as

if [ ! -e binutils-os_2-2.34 ]; then
    if [ ! -e binutils-os_2-2.34.tar.gz ]; then
        # Download tar.gz
        curl https://ftp.gnu.org/gnu/binutils/binutils-2.34.tar.gz --output binutils-os_2-2.34.tar.gz
    fi

    # Extract contents
    tar -xzvf binutils-os_2-2.34.tar.gz

    # Apply patch
    mv binutils-2.34 binutils-os_2-2.34
    cd binutils-os_2-2.34
    git init
    git apply ../binutils-os_2-2.34.patch
    git apply ../binutils-os_2-2.34-libtool.patch
    cd ..
fi

if [ $ONLY_DOWNLOAD_AND_EXTRACT ]; then 
    exit
fi

if [ ! -d build-binutils ]; then
    mkdir -p build-binutils
    cd build-binutils
    ../binutils-os_2-2.34/configure --target=$TARGET --prefix=$ROOT/toolchain/cross --disable-nls --with-sysroot=$ROOT/sysroot --disable-werror --enable-shared
    cd ..
fi


cd build-binutils
make -j5

# Install
mkdir -p $ROOT/toolchain/cross
make install
cd ..
