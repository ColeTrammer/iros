#!/bin/sh

PORT_NAME=vttest
SRC_DIR='vttest-20190710'
BUILD_DIR='build-vttest'

download() {
    # Download tar.gz
    curl https://invisible-island.net/datafiles/release/vttest.tar.gz --output vttest.tar.gz

    # Extract contents
    tar -xzvf vttest.tar.gz
}

patch() {
    git init
    git apply ../vttest.patch
}

configure() {
    ../vttest/configure --host=$HOST --prefix=/usr
}

clean() {
    make clean
}

build() {
    make -j5
}

install() {
    make install DESTDIR=$ROOT/sysroot -j5
}

. ../.build_include.sh