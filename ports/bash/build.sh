#!/bin/sh

export PORT_NAME=bash
export SRC_DIR='bash-5.0'
export BUILD_DIR='build-bash'

download() {
    # Download tar.gz
    curl https://ftp.gnu.org/gnu/bash/bash-5.0.tar.gz --output bash-5.0.tar.gz

    # Extract contents
    tar -xzvf bash-5.0.tar.gz
}

patch() {
    git init
    git apply ../bash-5.0.patch
}

configure() {
    ../bash-5.0/configure --host=$HOST --disable-nls --without-bash-malloc --prefix=/usr

    perl -p -i -e "s/#define CAN_REDEFINE_GETENV 1/\/* #undef CAN_REDEFINE_GETENV *\//" config.h
    perl -p -i -e "s/#define GETCWD_BROKEN 1/\/* #undef GETCWD_BROKEN *\//" config.h
}

build() {
    make -j5
}

install() {
    make install-strip DESTDIR=$ROOT/sysroot -j5
}

clean() {
    make clean
}

. ../.build_include.sh