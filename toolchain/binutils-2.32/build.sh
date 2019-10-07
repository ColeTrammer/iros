#!/bin/sh

# Download tar.gz
curl https://mirrors.syringanetworks.net/gnu/binutils/binutils-2.32.tar.gz --output binutils-os_2-2.32.tar.gz

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
# cd ../..
# make clean install-headers
# cd toolchain/binutils-2.32

# Build
mkdir -p build-binutils
cd build-binutils
../binutils-2.32/configure --target=x86_64-os_2 --prefix=/home/eloc/Workspace/os/os_2/toolchain/cross --disable-nls --with-sysroot=/home/eloc/Workspace/os/os_2/sysroot --disable-werror
make

# Install
mkdir -p ../../cross
make install
cd ..

# Clean Up
rm -rf build-binutils
rm -rf binutils-2.32
rm binutils-os_2-2.32.tar.gz
