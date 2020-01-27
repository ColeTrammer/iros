#!/bin/sh

set -e

# Variables
export ROOT="$PWD/../.."
export HOST=`$ROOT/default-host.sh`

if [ ! -d vttest ]; then
    # Download tar.gz
    curl https://invisible-island.net/datafiles/release/vttest.tar.gz --output vttest.tar.gz

    # Extract contents
    tar -xzvf vttest.tar.gz
    mv vttest-20190710 vttest

    # Apply patch
    cd vttest
    git init
    git apply ../vttest.patch
    cd ..

    # Install headers for correct build
    cd ../..
    make install-headers
    cd ports/vttest

    # Build
    mkdir -p build-vttest
    cd build-vttest
    ../vttest/configure --host=$HOST --prefix=/usr
    cd ..
fi

cd build-vttest
make clean
make -j5
make install DESTDIR=$ROOT/sysroot -j5
cd ..
