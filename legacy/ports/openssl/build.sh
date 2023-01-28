#!/bin/sh

export PORT_NAME=openssl
export SRC_DIR='openssl'
export BUILD_DIR='build-openssl'
export INSTALL_COMMAND=${INSTALL_COMMAND:-'install'}

export CFLAGS='-O2 -Wall -fno-omit-frame-pointer'

download() {
    git clone https://github.com/openssl/openssl --depth=1
}

patch() {
    git apply ../openssl.patch
}

configure() {
    "../$SRC_DIR/Configure" --prefix=/usr os_2-x86_64 -no-tests
}

build() {
    make -j5
}

. ../.build_include.sh
