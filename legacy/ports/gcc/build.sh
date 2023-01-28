#!/bin/sh

export PORT_NAME=gcc
export SRC_DIR='gcc-9.2.0'
export BUILD_DIR='build-gcc'
export INSTALL_COMMAND="${INSTALL_COMMAND:-install-gcc install-target-libgcc install-target-libstdc++-v3}"
export AUTO_CONF_OPTS='ac_cv_c_bigendian=no'
export MAKE_ARGS="$AUTO_CONF_OPTS"
export CFLAGS='-O2 -fno-omit-frame-pointer'
export CXXFLAGS='-O2 -fno-omit-frame-pointer'

download() {
    # Download tar.gz
    curl http://ftp.gnu.org/gnu/gcc/gcc-9.2.0/gcc-9.2.0.tar.gz --output gcc-os_2-9.2.0.tar.gz

    # Extract contents
    tar -xzvf gcc-os_2-9.2.0.tar.gz
}

patch() {
    git init
    git apply ../../../toolchain/gcc-9.2.0/gcc-os_2-9.2.0.patch
    git apply ../../../toolchain/gcc-9.2.0/gcc-9.2.0-customizations.patch
    git apply ../../../toolchain/gcc-9.2.0/gcc-9.2.0-shlib.patch
    git apply ../../../toolchain/gcc-9.2.0/gcc-9.2.0-libstdc++.patch


    ./contrib/download_prerequisites
    git apply ../gcc-deps.patch
    git apply ../gcc-deps-libtool.patch
}

configure() {
    export gcc_cv_initfini_array=yes
    ../gcc-9.2.0/configure \
        --host="$HOST" \
        --target="$HOST" \
        --prefix=/usr \
        --disable-nls \
        --disable-lto \
        --with-sysroot=/ \
        --with-build-sysroot="$ROOT/sysroot" \
        --enable-languages=c,c++ \
        --enable-shared \
        --enable-host-shared \
        --enable-threads=posix \
        --with-system-zlib
}

build() {
    make all-gcc all-target-libgcc all-target-libstdc++-v3 -j5 $AUTO_CONF_OPTS
}

. ../.build_include.sh
