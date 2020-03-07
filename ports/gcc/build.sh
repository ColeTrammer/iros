#!/bin/sh

set -e

# Variables
export ROOT="$PWD/../.."
export HOST=`$ROOT/default-host.sh`

if [ ! -e gcc-9.2.0 ]; then
    # Download tar.gz
    curl http://ftp.gnu.org/gnu/gcc/gcc-9.2.0/gcc-9.2.0.tar.gz --output gcc-os_2-9.2.0.tar.gz

    # Extract contents
    tar -xzvf gcc-os_2-9.2.0.tar.gz


    # Apply patch
    cd gcc-9.2.0
    ./contrib/download_prerequisites
    git init
    git apply ../gcc-os_2-9.2.0.patch
    git apply ../gcc-deps.patch
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
cd ports/gcc

if [ ! -e build-gcc ]; then
    mkdir -p build-gcc
    cd build-gcc
    ../gcc-9.2.0/configure --host=$HOST --target=$HOST --prefix=/usr --disable-nls --disable-lto --with-sysroot=/ --with-build-sysroot=$ROOT/sysroot --enable-languages=c,c++
    cd ..
fi

# Build
AUTO_CONF_OPTS='ac_cv_c_bigendian=no'

cd build-gcc
make clean
make all-gcc all-target-libgcc all-target-libstdc++-v3 -j5 $AUTO_CONF_OPTS
make install-strip-gcc install-strip-target-libgcc install-strip-target-libstdc++-v3 DESTDIR=$ROOT/sysroot -j5 $AUTO_CONF_OPTS
cd ..
