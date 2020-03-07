#!/bin/sh

set -e

# Variables
export ROOT="$PWD/../.."
export HOST=`$ROOT/default-host.sh`

if [ ! -d make-4.3 ]; then
    # Download tar.gz
    curl https://ftp.gnu.org/gnu/make/make-4.3.tar.gz --output make-4.3.tar.gz

    # Extract contents
    tar -xzvf make-4.3.tar.gz

    # Apply patch
    cd make-4.3
    git init
    git apply ../make-4.3.patch
    cd ..
fi

# Install headers for correct build
cd ../..
make install-headers
cd ports/make
rm $ROOT/sysroot/usr/include/dlfcn.h

if [ ! -e build-make ]; then
    mkdir -p build-make
    cd build-make
    ../make-4.3/configure --host=$HOST --disable-nls --prefix=/usr --without-guile
    cd ..
fi

# # Build
cd build-make
make clean
make -j5
make install-strip DESTDIR=$ROOT/sysroot -j5
cd ..