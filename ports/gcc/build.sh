#!/bin/sh

download() {
    # Download tar.gz
    curl http://ftp.gnu.org/gnu/gcc/gcc-9.2.0/gcc-9.2.0.tar.gz --output gcc-os_2-9.2.0.tar.gz

    # Extract contents
    tar -xzvf gcc-os_2-9.2.0.tar.gz
}

patch() {
    ./contrib/download_prerequisites
    git init
    git apply ../gcc-os_2-9.2.0.patch
    git apply ../gcc-deps.patch

    cd libstdc++-v3
    autoconf
    cd ..
}

configure() {
    ../gcc-9.2.0/configure --host=$HOST --target=$HOST --prefix=/usr --disable-nls --disable-lto --with-sysroot=/ --with-build-sysroot=$ROOT/sysroot --enable-languages=c,c++
}

export AUTO_CONF_OPTS='ac_cv_c_bigendian=no'

clean() {
    make clean
}

build() {
    make all-gcc all-target-libgcc all-target-libstdc++-v3 -j5 $AUTO_CONF_OPTS
}

install() {
    make install-strip-gcc install-strip-target-libgcc install-strip-target-libstdc++-v3 DESTDIR=$ROOT/sysroot -j5 $AUTO_CONF_OPTS
}

. ../.build_include.sh