#!/bin/sh

PORT_NAME=curl
SRC_DIR='curl-7.71.1'
BUILD_DIR='build-curl'

export CFLAGS='-O2 -fno-omit-frame-pointer'
export curl_disallow_getaddrinfo=yes # currently, getaddrinfo() doesn't support service lookup

download() {
    # Download tar.gz
    curl https://curl.haxx.se/download/curl-7.71.1.tar.gz.asc --output curl-7.71.1.tar.gz

    # Extract contents
    tar -xzvf curl-7.71.1.tar.gz
}

patch() {
    git init
    git apply ../curl-7.71.1.patch
}

configure() {
    ../"$SRC_DIR"/configure --host=$HOST --prefix=/usr --with-shared --disable-ipv6
}

. ../.build_include.sh
