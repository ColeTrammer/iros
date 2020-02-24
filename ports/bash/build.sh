#!/bin/sh

set -e

# Variables
export ROOT="$PWD/../.."
export HOST=`$ROOT/default-host.sh`

if [ ! -d bash-5.0 ]; then
    # Download tar.gz
    curl https://ftp.gnu.org/gnu/bash/bash-5.0.tar.gz --output bash-5.0.tar.gz

    # Extract contents
    tar -xzvf bash-5.0.tar.gz

    # Apply patch
    cd bash-5.0
    git init
    git apply ../bash-5.0.patch
    cd ..

fi

# Install headers for correct build
cd ../..
make install-headers
cd ports/bash

if [ ! -e build-bash ]; then
    mkdir -p build-bash
    cd build-bash
    ../bash-5.0/configure --host=$HOST --disable-nls --without-bash-malloc --prefix=/usr

    perl -p -i -e "s/#define CAN_REDEFINE_GETENV 1/\/* #undef CAN_REDEFINE_GETENV *\//" config.h
    perl -p -i -e "s/#define GETCWD_BROKEN 1/\/* #undef GETCWD_BROKEN *\//" config.h

    cd ..
fi

# Build
cd build-bash
make clean
make -j5
make install-strip DESTDIR=$ROOT/sysroot -j5
cd ..
