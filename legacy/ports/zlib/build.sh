#!/bin/sh

PORT_NAME=zlib
SRC_DIR='zlib-1.2.11'
BUILD_DIR='build-zlib'

export CHOST=x86_64-os_2

download() {
    # Download tar.gz
    curl https://zlib.net/zlib-1.2.11.tar.gz --output zlib-1.2.11.tar.gz

    # Extract contents
    tar -xzvf zlib-1.2.11.tar.gz
}

patch() {
    git init
    git apply ../zlib.patch
}

configure() {
    ../"$SRC_DIR"/configure --prefix=/usr
}

. ../.build_include.sh
