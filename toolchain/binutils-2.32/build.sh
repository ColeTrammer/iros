#!/bin/sh

# Variables
export ROOT="$PWD/../.."
export TARGET=`$ROOT/default-host.sh`

# Download tar.gz
curl https://ftp.gnu.org/gnu/binutils/binutils-2.32.tar.gz --output binutils-os_2-2.32.tar.gz

# Extract contents
tar -xzvf binutils-os_2-2.32.tar.gz

# Apply patch
cd binutils-2.32
git init
git apply ../binutils-os_2-2.32.patch
cd ..

# Run automake
cd binutils-2.32
cd ld
automake-1.15
cd ../..

# Install headers for correct build
cd ../..
make clean install-headers
cd toolchain/binutils-2.32

# Build
mkdir -p build-binutils
cd build-binutils
../binutils-2.32/configure --target=$TARGET --prefix=$ROOT/toolchain/cross --disable-nls --with-sysroot=$ROOT/sysroot --disable-werror
make -j5

# Install
mkdir -p $ROOT/toolchain/cross
make install
cd ..
